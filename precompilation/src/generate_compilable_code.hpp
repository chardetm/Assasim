/**
 * \file generate_compilable_code.hpp
 * \brief Generates the methods that are needed in order for the final model to
 *        compile and work.
 *
 * \todo Generate the implementation of the virtual function defined in Agent.hpp regarding received Interactions.
 * \todo Be able to parse a partial declaration of an agent and complete the
 *       missing arguments by their default values.
 * \todo Rewrite the Behavior defined by the user to fit with all the functions
 *       in agent.hpp.
 * \todo Adapt functions called by the user to the syntax of agent.hpp for SendMessage.
 * \todo Generation of Agent::GetJsonNode.
 */

#ifndef GENERATE_COMPILABLE_CODE_HPP_
#define GENERATE_COMPILABLE_CODE_HPP_

#include "utils.hpp"
#include "model.hpp"


/**
 * Generates the complete constructor of each agent (initializes all sendable
 * attributes as well as meta attributes).
 */
std::string GenerateAgentConstructor(Model &model);

/**
 * Generates the method ReceiveMessage (depends on the interactions defined in
 * in the model but is common to all agents) which informs the agent of the
 * arrival of an interaction. Duplicates the same method for all types of agents.
 */
std::string GenerateAgentReceiveMessage(Model &model);

/**
 * Generates the method ResetMessages (depends on the interactions defined in
 * in the model but is common to all agents) which deletes all messages
 * received and treated during the previous execution of Behavior.  Duplicates
 * the same method for all types of agents.
 */
std::string GenerateAgentResetMessages(Model &model);

/**
 * Generates the function GetPointerToAttribute which returns a pointer to the
 * attribute which id is given as input.
 */
std::string GenerateAgentGetPointerToAttribute(Model &model);

/**
 * Generates the function SetAttributeValue which modifies the value of the
 * public attribute which id is given as input to the value written in the
 * memory location also given as input.
 */
std::string GenerateAgentSetAttributeValue(Model &model);

/**
 * Generates the function CheckModifiedCriticalAttributes which fills
 * updated_critical_attributes_ with the critical attributes that were modified
 * during the previous behavior.
 */
std::string GenerateAgentCheckModifiedCriticalAttributes(Model &model);

/**
 * Generates the function CopyPublicAttributes which copies the public structure
 * of the agent in the given memory location.
 */
std::string GenerateAgentCopyPublicAttributes(Model &model);

/**
 * Generates the function Agent::CopyCriticalAttributes which copies the
 * critical structure of the agent in the given memory location.
 */
std::string GenerateAgentCopyCriticalAttributes(Model &model);

/**
 * Generates the function Agent::FromStruct which returns a pointer to the good
 * type of agent whose attributes are initilized using a struct of attributes
 * given in argument.
 */
std::string GenerateAgentFromStruct(Model &model);

/**
 * Generates the function Agent::CreateStruct which fills the internal private
 * structure_ to a structure representing all the attributes of the Agent
 */
std::string GenerateAgentCreateStruct(Model &model);

/**
 * Generates the function Agent::GetJsonNode which returns a UBjson
 * representation of all the attributes of the Agent
 * \todo Implement this function
 * \todo Allow to send only some of the attributes
 */
std::string GenerateAgentGetJsonNode(Model &model);

/**
   Generates the function CreateStruct for each interaction wich fill the private
   attribute structure_ of the interaction.
 */
std::string GenerateInteractionCreateStruct(Model &model);

/**
 * Generates the function Interaction::FromStruct which return a pointer to the
 * good type of agent whose attributes are initilized using a struct of
 * attributes given in argument.
 */
std::string GenerateInteractionFromStruct(Model &model);

/**
   Adds the vector of received interaction for each type in each agent
 */
void AddReceivedInteractionsInAgents(Model &model, clang::Rewriter &rewriter);

/**
   For each user-defined constructor in each interaction, replace it with an identical 
   constructor taking the 5 arguments used to identify the type, sender and recipient of the interaction.
 */
void AddConstructorsInInteractionsStep2(Model &model, clang::Rewriter &rewriter);

/**
   Adds the prototypes for the abstract methods defined in class Agent and the complete constructor.
 */
void AddPrototypesInAgentsStep2(Model &model, clang::Rewriter &rewriter);

/**
 * Generates the content of the file which implements the methods defined in agent.hpp.
 */
// TODO: remove the "output_folder" arguement?
std::string GenerateAgentCpp(Model &model);

/**
 * Generates the content of the file which implements the functions handling model-specific commands for the CLI
 */
// TODO: remove the "output_folder" arguement?
std::string GenerateUserInterfaceModelCpp(Model &model);

#endif
