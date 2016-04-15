/**
 * \file user_interface.hpp
 * \brief Defines the functions used to parse the commands of the user and
 *        control the masters.
 */

#ifndef USER_INTERFACE_HPP_
#define USER_INTERFACE_HPP_

#include <string>
#include <boost/interprocess/ipc/message_queue.hpp>
#include "master.hpp"


/**
 * \enum Control
 * \brief Keywords used to communicate between Control and CommandLine.
 */
enum class Control {
	/// Control command used to pause the simulation.
	IDLE,

	/// Control command used to initialize the masters on all processes.
	INIT,

	/// Control command used to make the simulation run until it not paused.
	RUN,

	/// Control command used to modify on all processes the number of threads
	/// allocated to each master.
	CHANGE_NB_THREADS,

	/// Control command used to quit the command line interface.
	EXIT
};

// Error messages
/// Error launched when the simulation has to be initiated to execute the command.
const std::string error_init = "No simulation has been initiated. Execute first a 'init'.\n";

/// Error launched when the simulation has to be cleared to execute the command.
const std::string error_reset = "This can only be done once the simulation is cleared.\n";

/// Error launched when the input command in not known.
const std::string inv_com = "Invalid command. Enter help for more information.\n";

/**
 * \fn MasterHandler(int rank)
 * \brief Handles each masters which is not master 0 while master 0 is handled
 *        in CommandLine().
 * \param rank Rank of the MPI calling process.
 */
void MasterHandler(int rank);

/**
 * \fn void Parse(const char* buffer, Control &control, int &nb_threads, int &nb_masters, bool &is_alive)
 * \brief Parses the input of the command line and launches the corresponding
 *        orders.
 * \param buffer Content of the command line.
 * \param control Reference to the control variable of process 0.
 * \param nb_threads Reference to the nb_threads variable of process 0.
 * \param nb_masters Reference to the nb_masters variable of process 0.
 * \param is_alive Reference to the is_alive variable of process 0.
 */
void Parse(const char* buffer, Control &control, int &nb_threads, int &nb_masters, bool &is_alive);


void Listen();

/**
 * \fn void InitUserInterface()
 * \brief Calls Listen on process 0 and MasterHandler on the others.
 */
void InitUserInterface(std::string queue_id);

#endif
