/**
 * \file heuristics.cpp
 * \brief Implements the heuristics of heuristics.hpp.
 */


#include "heuristics.hpp"


void NaiveInitialMastersAssignement(
	std::vector<void*> &initial_agents,
	std::vector<MasterId> &assignment, MasterId nb_masters)
{
	size_t nb_agents = initial_agents.size();
	for (size_t k=0; k<nb_agents; k++) {
		assignment[k] = k%nb_masters;
	}
}


void AssignInitialMasters(
	std::vector<void*> &initial_agents,
	std::vector<MasterId> &assignment, MasterId nb_masters)
{
	NaiveInitialMastersAssignement(initial_agents, assignment, nb_masters);
}


void NaiveInitialAgentHandlersAssignement(
   utils::fixed_size_multibuffer<AgentStruct> &initial_agents,
   std::vector<size_t> &assignment, size_t nb_agent_handlers)
{
	size_t nb_agents = initial_agents.size();
	for (size_t k=0; k<nb_agents; k++) {
		assignment[k] = k%nb_agent_handlers;
	}
}


void AssignInitialAgentHandlers(
   utils::fixed_size_multibuffer<AgentStruct> &initial_agents,
   std::vector<size_t> &assignment, size_t nb_agent_handlers)
{
	NaiveInitialAgentHandlersAssignement(initial_agents, assignment, nb_agent_handlers);
}

void MigrateAgents() {
	// Fill MetaEvolutionDescriptions with all the migrations needed, using a given heuristic
}
