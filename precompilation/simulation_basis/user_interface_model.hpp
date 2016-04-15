/**
 * \file user_interface.hpp
 * \brief Defines the function regarding the CLI which are specific to the model.
 *
 * The implementation of these functions is generated during the precompilation
 * step.
 */


#ifndef USER_INTERFACE_MODEL_HPP_
#define USER_INTERFACE_MODEL_HPP_

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <mpi.h>
#include <vector>
#include <string>

#include <readline/readline.h>
#include <readline/history.h>

#include "master.hpp"


/**
 * \brief Exception thrown when an error occurs while parsing an instance JSON file (in Instanciate).
 */
class InstanciateException : public std::runtime_error {
public:
	InstanciateException(const std::exception &e)
	: std::runtime_error{std::string("Error while parsing the JSON file describing an instance. Please check the format of the file.\n\nUnderlying error: ") + e.what()}
	{}
	InstanciateException(const std::string &str)
	: std::runtime_error{std::string("Error while parsing the JSON file describing an instance. Please check the format of the file.\n\nUnderlying error: ") + str}
	{}
};


/**
 * \fn std::vector<void*> Instanciate(std::string file)
 * \brief Takes as input a file and outputs the corresponding instanciation of
 * the agents.
 * \param file Path to the file to open.
 * \return A vector of pointers to the AgentStructs representing the agents
 *         described in the input file.
 * \todo The format of the input file is JSON.
 */
std::vector<void*> Instanciate(std::string file);

/// Model specific commands
const std::vector<const char*> model_commands = {
	"print_model",
	"print_agent"
};

/**
 * Handle the model specific commands
 */
void ParseModelCommand(const char *buffer, std::unique_ptr<Master> &root_master, bool is_alive);

#endif
