/**
 * \file model_environment.hpp
 * \brief Defines the first step of precompilation: building the C++ environment
 *        needed for the user implementation Behaviors.
 */

#ifndef MODEL_ENVIRONMENT_HPP_
#define MODEL_ENVIRONMENT_HPP_

#include <cstring>
#include <map>
#include <sstream>

#include "clang/Rewrite/Core/Rewriter.h"

#include "utils.hpp"
#include "analyze_class.hpp"
#include "model.hpp"
#include "build_model.hpp"

/**
 * Adds complete constructor in the declaration of each interaction.
 */
void AddConstructorInInteraction(Model &model, clang::Rewriter &rewriter);

/**
 * For each attribute of each agent of model, adds to the agent declaration the
 * getter for this attribute.
 * \deprecated This function is not useful anymore.
 */
void AddGetterInAgents(Model &model, clang::Rewriter &rewriter);

/**
 * Adds void implementation of Behavior method file behaviors.cpp
 */
std::string GenerateBehaviorsContent(Model &model, clang::Rewriter &rewriter);

/**
 * Generates the content of the new agent.hpp. The new class Agent contains a list
 * of received messages for each Interaction type and other global parameters of
 * the simulation.
 */
std::string GenerateAgentHeaderContent(Model &model);

/// Macro defining the suffix for the const value containing the type id of each interaction and agent
#define TYPETAG "_type"

/**
   Generates the content of the new consts.hpp which defines the macro for Agent and Interaction type recognition.
 */
std::string GenerateConstsHeaderContent(Model &model);

/**
 * Generates the content of the new interaction.hpp (at this step, it is just void).
 */
std::string GenerateInteractionHeaderContent();

/**
 * Generates the code for simplified syntax for accessing public attributes of agents.
 */
std::string GenerateAgentDataAccessStep1(Model &model);

/**
 * Performs code generation for agents and interactions from build_model.
 */
void ConstructEnvironment(Model &model, clang::Rewriter &rewriter);

#endif
