/**
 * \file command_line_interface.cpp
 * \brief Implements the command line interface for the simulation.
 */

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>
#include <mpi.h>
#include <memory>
#include <boost/interprocess/ipc/message_queue.hpp>

#include <readline/readline.h>
#include <readline/history.h>

std::unique_ptr<boost::interprocess::message_queue> mq_orders;
const char *run_cmd = "run";
const char *exit_cmd = "exit";
char buffer[1024];

const char *help_msg = "Available commands:\n\
  + set_period <number>: determine how many step of the simulation is done\n\
  + set_nb_threads <number>: determine how many threads are used - for each computing unit\n\
  + init <json_file>: initialize the simulation by loading the instanciation in the file given in options\n\
  + run (<number_of_steps>): run the simulation for period*number_of_steps. If the number of steps is not specified, run the simulation until receiving an order\n\
  + pause: pause the simulation\n\
  + kill: completely stop the simulation, freeing memory\n\
  + help: print this help message\n\
  + export_json <file.json>: export the snapshot of the state of the simulation in json\n\
  + export_ubjson <file.json>: export the snapshot of the state of the simulation in binary json\n\
  + convert <snapshot_init.json> <instance_output.json>: convert a file exported by the simulation to a file that can be given as initialisation\n\
  + quit/exit: kill the simulation and quit the program.";

/// List of all commands or keywords.
const std::vector<const char*> commands = {
	"quit",
	"exit",
	"init",
	"run",
	"pause",
	"kill",
	"set_period",
	"set_nb_threads",
	"export_json",
	"export_ubjson",
	"convert",
	"help"
};

std::string mq_name;

char* CompletionGenerator(const char* stem_text, int state);


/**
 * \fn char** CommandCompletion(const char* stem_text, int start, int end)
 * \brief Performs auto completion given the input.
 * \param stem_text Text given as input.
 * \param start Index of the position of stem_text on the command line.
 * \param end Index of the ending of stem_text on the command line.
 * \return An array of the completion matches.
 */
char** CommandCompletion(const char* stem_text, int start, int end) {
    char ** matches = nullptr;
	// Generates autocompletion only for commands that are on the beginning of
	// the line
    if (start == 0) {
        matches = rl_completion_matches(stem_text, CompletionGenerator);
    }
    return matches;
}


/**
 * \fn char* CompletionGenerator(const char* stem_text, int state)
 * \brief Returns at each call a potential match for the input.
 * \param stem_text Text given as input.
 * \param state Number of previous calls to this functions.
 * \return Returns a potential match for stem_text, and nullptr if there is no
 *         other possible match.
 */
char* CompletionGenerator(const char* stem_text, int state) {
    static int count;

    if (state == 0) {
        count = -1;
    }

    int text_len = strlen(stem_text);

    // Searches through commands until it finds a match
    while (count < (int)commands.size()-1) {
		count++;
        if (strncmp(commands[count], stem_text, text_len) == 0) {
            // Returns a duplicate of the possible completion
            return strdup(commands[count]);
        }
    }

    // There are no more matches
    return nullptr;
}


/**
 * \fn int RunSimulation()
 * \brief Function continuously called while waiting for an input in the CLI.
 *        Orders the simulator to run a period of the simulation.
 * \return 0
 */
int RunSimulation() {
	// TODO: Send message run to the simulation
	mq_orders->send(run_cmd, sizeof(run_cmd), 0);
	return 0;
}


/**
 * \fn void CommandLine()
 * \brief Handles the command lie interface using readline.
 */
void CommandLine() {
    // Use the custom autocompletion
    rl_attempted_completion_function = CommandCompletion;
    // Makes readline handle quotes and spaces
    rl_completer_quote_characters = strdup("\"\' ");

	// Line entered by the user
    char* buffer;
    while ((buffer = readline("assasim> ")) != nullptr) {
        if (strcmp(buffer, "") != 0) {
            add_history(buffer);
		}
		else {
			continue;
		}
		std::istringstream input(buffer);
		std::string command;
		input >> command;
		if (command == "help") {
			std::cerr << help_msg << "\n";
		}
		else {
			std::string temp;
			// Check that the correct number of arguments is passed to each command
			if (command == "set_period" || command == "set_nb_threads" || command == "init" || command == "export_json" || command == "export_ubjson") {
				if (!(input >> temp)) {
					std::cerr << "Wrong number of arguments! See help for further details." << std::endl;
					continue;
				}
			} else if (command == "convert") {
				if (!(input >> temp) || !(input >> temp)) {
					std::cerr << "Wrong number of arguments! See help for further details." << std::endl;
					continue;
				}
			} else if (command != "run" && command != "pause" && command != "kill" && command != "help" && command != "quit" && command != "exit") {
				std::cerr << "Unknown command. See help for list of available commands." << std::endl;
				continue;
			}

			// Send a message to the simulation
			mq_orders->send(buffer, (strlen(buffer)+1)*sizeof(char), 0);
			if (command == "exit" ||  command == "quit") {
				break;
			}
		}

        free(buffer);
        buffer = nullptr;
    }

    free(buffer);
	boost::interprocess::message_queue::remove(mq_name.c_str());
}


/**
 * \fn int main(int argc, char* argv[])
 * \brief Main function of the command line interface.
 * \param argc Number of argument given to the executable; should be equal to 3.
 * \param argv Arguments given to the executable; there should be two arguments:
 *        the location of the executable of the simulation, and the number of
 *        masters.
 * \details Verifies that it is called on only one process, start the simulation
 * and calls CommandLine.
 */
int main(int argc, char* argv[]) {
	if (argc != 3 && argc != 1) {
		std::cerr << "Not the good number of arguments\n";
		exit(EXIT_FAILURE);
	}

	srand(time(0));
	std::stringstream s; s << rand();

	int provided;
	// MPI is told to handle threads
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

	// The program checks that the CLI is not called on more than one process.
	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	if (size > 1) {
		int rank;
		MPI_Comm_size(MPI_COMM_WORLD, &rank);
		if (rank == 0) {
			std::cerr << "Error: the command line interface should not be called on more than one process" << std::endl;
		}
		MPI_Finalize();
		exit(EXIT_FAILURE);
	}

	// Lauching the simulation
	// TODO: Check if it works
	MPI_Comm common_comm;

	char *input = (char*)malloc((1+s.str().length())*sizeof(char));
	for (unsigned int i = 0; i<s.str().length(); i++)
		input[i] = s.str()[i];
    input[s.str().length()] = '\0';
	char* argv2[2] = {input,nullptr};

	s.str(""); s << "assasim_" << input;
	mq_name = s.str();

	boost::interprocess::message_queue::remove(mq_name.c_str());
	mq_orders = std::make_unique<boost::interprocess::message_queue>(boost::interprocess::create_only, mq_name.c_str(), 100, 1023);

	if (argc == 1) {
		std::cerr << "Interface launched in not-spawning mode. Communication token: " << input << "\n";
	}
	else  {
		MPI_Comm_spawn(argv[1], argv2, atoi(argv[2]), MPI_INFO_NULL, 0, MPI_COMM_SELF, &common_comm, MPI_ERRCODES_IGNORE);
		std::cerr << "Interface launched and simulation " << argv[1] << " spawned on " << atoi(argv[2]) << " processors.\n";
	}

	free(input);
	// Command Line Interface
	CommandLine();


	MPI_Finalize();
}
