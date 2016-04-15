/**
 * \file master_initialization.hpp
 * \brief Handles the generation of codes for the initialization of masters.
 *
 */

#ifndef MASTER_INITIALIZATION_HPP_
#define MASTER_INITIALIZATION_HPP_

#include <string>

#include "utils.hpp"
#include "model.hpp"

/**
 * Generates the attributes structs for each agent and interaction
 */
std::string GenerateAttributesStruct(Model &model);

/**
 * Generates the code for loading MPI_Datatypes of agent attributes
 */
std::string GenerateAttributesMPIDatatypesFunction(Model &model);

/**
 * Generates the code for loading MPI_Datatypes of agents
 */
std::string GenerateAgentsMPIDatatypesFunction(Model &model);

/**
 * Generates the code for loading MPI_Dataypes of struct containing critical attributes
 */
std::string GenerateCriticalStructsMPIDatatypesFunction(Model &model);

/**
 * Generates the code for loading MPI_Datatypes of interactions
 */
std::string GenerateInteractionsMPIDatatypesFunction(Model &model, clang::ASTContext *context);

/**
 * Generates the code for loading sizes of the attributes of agents
 */
std::string GenerateAttributesSizeFunction(Model &model);

/**
 * Generates the code for loading critical attributes
 */
std::string GenerateCriticalAttributesFunction(Model &model);

/**
 * Generates the code for loading non sendable agent types
 */
std::string GenerateNonSendableAgentTypesFunction(Model &model);

/**
 * Generates the code for loading public attributes offsets
 */
std::string GeneratePublicAttributesOffsetsFunction(Model &model);

/**
 * Generates the code for loading the size of the public structure of agents' attributes
 */
std::string GeneratePublicStructSizesFunction(Model &model);

/**
 * Generates the code for loading offsets of critical attributes (in the critical attributes struct)
 */
std::string GenerateCriticalAttributesOffsetsFunction(Model &model);

/**
 * Generates the code for loading the size of the critical structure of agents' attributes
 */
std::string GenerateCriticalStructSizesFunction(Model &model);

/**
 * Generates the code that will build the relation between agent types and names
 * (strings) in the agent.
 */
std::string GenerateAgentsNamesRelation(Model &model);

/**
 * Generates the code that will build the relation between attributes and names
 * (strings) in the agent.
 */
std::string GenerateAttributesNamesRelation(Model &model);

/**
 * Generates the code that returns the (constant) number of agent classes
 */
std::string GenerateNbAgentTypesFunction(Model &model);

/**
 * Generates the code that returns the (constant) number of interaction classes
 */
std::string GenerateNbInteractionTypesFunction(Model &model);

/**
 * Generates the content of the file containing all the generated structs
 */
std::string GenerateStructFile(Model &model);

/**
 * Generates the complete code for initialization of master data
 */
std::string GenerateMasterInitialization(Model &model, clang::ASTContext *context);

#endif
