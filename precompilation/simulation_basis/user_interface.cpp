/**
 * \file user_interface.cpp
 * \brief Implements the functions used to parse the commands of the user and
 *        control the masters.
 *
 * \todo Complete the list of commands.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <mpi.h>
#include <algorithm>
#include <chrono>
#include <thread>

#include "libs/ubjsoncpp/include/stream_writer.hpp"

#include <readline/readline.h>
#include <readline/history.h>

#include "user_interface_model.hpp"
#include "user_interface.hpp"
#include "master.hpp"

// Control variable
Control control = Control::IDLE;

// Master used in the current MPI process
std::unique_ptr<Master> master;

std::unique_ptr<boost::interprocess::message_queue> mq_orders;
const char *done_cmd = "done";
const char *error_cmd = "error";
std::string mq_name;
bool run;

void MasterHandler(int rank) {

	// Control variable
	Control control = Control::IDLE;

	// Number of threads for each process
	int nb_threads = 2;

	// Number of masters
	int nb_masters;
	// For the moment the number of masters is fixed but it could vary
	MPI_Comm_size(MPI_COMM_WORLD, &nb_masters);

	while (control != Control::EXIT) {
		switch (control) {
			case Control::INIT: {
				std::vector<void*> artefact; // Virtual vector
				master.reset();
				master = std::make_unique<Master>(rank, nb_masters, nb_threads, artefact);
				master->WaitOrderFromRoot();
				break;
			}
			case Control::CHANGE_NB_THREADS: {
				MPI_Bcast(&nb_threads, 1, MPI_INT, 0, MPI_COMM_WORLD);
				break;
			}
			default:
				break;
		}
		// Waits for a control from process 0
		MPI_Bcast(&control, 1, MPI_INT, 0, MPI_COMM_WORLD);

	}

}


void Parse(const char* buffer, Control &control, int &nb_threads, int &nb_masters, bool &is_alive) {

	std::istringstream input(buffer);
	std::string command; input >> command;
	if (command == "quit" || command == "exit") {
		control = Control::EXIT;
		if (is_alive) {
			master->KillSimulation();
		}
		MPI_Bcast(&control, 1, MPI_INT, 0, MPI_COMM_WORLD);
	} else if (command == "init") {
		control = Control::INIT;
		if (is_alive) {
			master->KillSimulation();
			master.reset();
		}
		MPI_Bcast(&control, 1, MPI_INT, 0, MPI_COMM_WORLD);
		std::string file; input >> file;
		// FIXME: Uncomment Instanciate when it is done
		std::vector<void*> instanciation;// = Instanciate(file);
		if (file != "")
			instanciation = Instanciate(file);
		master = std::make_unique<Master>(0, nb_masters, nb_threads, instanciation);
		is_alive = true;
		// Freeing of the initialisation
		// FIXME: make the free work
		for (auto &x : instanciation) {
			//free(x);
		}
	} else if (command == "run") {
		if (is_alive) {
			control = Control::RUN;
			int n_steps;
			if (input >> n_steps) {
				// Run for the number of steps specified
				for (int k = 0; k<n_steps; k++)
					master->RunSimulation();
			}
			else {
				run = true;
			}
		} else {
			std::cerr << error_init;
		}

	} else if (command == "pause") {
		control = Control::IDLE;
	} else if (command == "kill") {
		if (is_alive) {
			master->KillSimulation();
			is_alive = false;
		}
	} else if (command == "set_period") {
		if (is_alive) {
			Time new_period; input >> new_period;
			master->ChangePeriod(new_period);
		}
	} else if (command == "set_nb_threads") {
		if (is_alive) {
			std::cerr << error_reset;
		} else {
			// Sending the command
			control = Control::CHANGE_NB_THREADS;
			MPI_Bcast(&command, 1, MPI_INT, 0, MPI_COMM_WORLD);
			// Sending the new number of threads
			input >> nb_threads;
			MPI_Bcast(&nb_threads, 1, MPI_INT, 0, MPI_COMM_WORLD);
		}
	} else if (command == "export_json") {
		if (is_alive) {
			ubjson::Value json = master->ExportSimulation();
			std::string output; input >> output;
			std::ofstream file(output);
			file << ubjson::to_ostream(json, ubjson::to_ostream::pretty) << std::endl;
			file.close();
		} else {
			std::cerr << error_init;
		}
	} else if (command == "convert") {
		if (is_alive) {
			ubjson::Value ubjson = master->ExportSimulation();
			std::string in; input >> in;
			std::string out; input >> out;
			master->ConvertOutputToInput(in, out);
		} else {
			std::cerr << error_init;
		}
	} else if (std::find(model_commands.begin(), model_commands.end(), command) != model_commands.end()) {
		// It is a model-specific command
		ParseModelCommand(buffer, master, is_alive);
	} else {
		std::cerr << inv_com;
	}

}

void Listen() {
	// Number of threads for each process
	int nb_threads = 2;

	// Number of masters
	int nb_masters;
	// For the moment the number of masters is fixed but it could vary
	MPI_Comm_size(MPI_COMM_WORLD, &nb_masters);

	// Indicates if master 0 is instanciated
	bool is_alive = false;

	boost::interprocess::message_queue::size_type recvd_size;
	unsigned int priority;
	char buffer[1024];
	while (control != Control::EXIT) {
		if (mq_orders->try_receive(buffer, 1024, recvd_size, priority)) {
			run = false;
			Parse(buffer, control, nb_threads, nb_masters, is_alive);
		}
		else if (run)
			master->RunSimulation();
	}
}


void InitUserInterface(std::string queue_id) {
	int rank;
	run =false;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	if (rank == 0) {
		// Open the message queue for user interaction
		bool first = true;
		while (control != Control::EXIT) {
			try {
				mq_name = "assasim_"+queue_id;
				mq_orders = std::make_unique<boost::interprocess::message_queue>
					(boost::interprocess::open_only
					 ,mq_name.c_str()
						);
				Listen();
			}
			catch (boost::interprocess::interprocess_exception &ex){
				if (std::string(ex.what()) == "No such file or directory") {
					if (first) {
						std::cerr << "No interface found. Waiting for interface..." << std::endl;
						first = false;
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					continue;
				}
				else {
					std::cerr << "Error while opening interface: " << ex.what();
					exit(1);
				}
			}
		}
	} else {
		MasterHandler(rank);
	}
}
