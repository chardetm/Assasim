/**
 * \file heuristics.hpp
 * \brief Defines the heuristics that will be used for the repartition of agents
 *        between masters (initial repartition and migration).
 *
 * \todo Design intelligent heuristics.
 */

#ifndef HEURISTICS_HPP_
#define HEURISTICS_HPP_

#include <vector>

#include "agent.hpp"


/**
 * \fn void NaiveInitialMastersAssignement(std::vector<void*> &initial_agents,
 *                                         std::vector<MasterId> &assignment, MasterId nb_masters)
 * \brief Allocates agents to masters so that all masters get the same amount of
 *        agents.
 * \param initial_agents Reference to the vector of pointers to AgentStructs
 *        representing the initial agents.
 * \param assignment Reference to the vector which will contain the result of
 *        the assignment.
 * \param nb_masters Number of masters in the simulation.
 * \see AssignInitialMasters.
 * \pre The size of assignment must be the same as initial_agents.
 */
void NaiveInitialMastersAssignement(
	std::vector<void*> &initial_agents,
	std::vector<MasterId> &assignment, MasterId nb_masters);

/**
 * \fn void AssignInitialMasters(std::vector<void*> &initial_agents,
 *                               std::vector<MasterId> &assignment, MasterId nb_masters)
 * \brief Assigns the initial agents to their initial masters. May be able to
 *        choose the best heuristic for this choice.
 * \param initial_agents Reference to the vector of pointers to AgentStructs
 *        representing the initial agents.
 * \param assignment Reference to the vector which will contain the result of
 *        the assignment.
 * \param nb_masters Number of masters in the simulation.
 * \details Fills assignment such that agent initial_agents[i] will be given to
 * master assignment[i] and nb_to_send[j] contains the number of agents which
 * will be given to master j.
 * \pre The size of assignment must be the same as initial_agents.
 */
void AssignInitialMasters(
	std::vector<void*> &initial_agents,
	std::vector<MasterId> &assignment, MasterId nb_masters);

/**
 * \pre void NaiveInitialAgentHandlersAssignement(
 *               utils::fixed_size_multibuffer<AgentStruct> &initial_agents,
 *               std::vector<size_t> &assignment, size_t nb_agent_handlers)
 * \brief Allocates agents to agent handlers so that all agent handlers get the
 *        same amount of agents.
 * \param initial_agents Reference to the fixed_size_multibuffer containing the
 *        AgentStructs representing the initial agents.
 * \param assignment Reference to the vector which will contain the result of
 *        the assignment.
 * \param nb_agent_handlers Number of agent handlers on the current master.
 * \see Used in AssignInitialAgentHandlers.
 * \pre The size of assignment must be the same as initial_agents.
 */
void NaiveInitialAgentHandlersAssignement(
	utils::fixed_size_multibuffer<AgentStruct> &initial_agents,
	std::vector<size_t> &assignment, size_t nb_agent_handlers);

/**
 * \fn void AssignInitialAgentHandlers(
 *              utils::fixed_size_multibuffer<AgentStruct> &initial_agents,
 *              std::vector<size_t> &assignment, size_t nb_agent_handlers)
 * \param initial_agents Reference to the fixed_size_multibuffer containing the
 *        AgentStructs representing the initial agents.
 * \param assignment Reference to the vector which will contain the result of
 *        the assignment.
 * \param nb_agent_handlers Number of agent handlers on the current master.
 * \details Fills assignment such that agent initial_agents[i] will be given to
 * agent handler assignment[i].
 * \pre The size of assignment must be the same as initial_agents.
 */
void AssignInitialAgentHandlers(
	utils::fixed_size_multibuffer<AgentStruct> &initial_agents,
	std::vector<size_t> &assignment, size_t nb_agent_handlers);

/**
 * Fills MetaEvolutionDescriptions with all the migrations needed, using a given heuristic
 */
void MigrateAgents();

#endif
