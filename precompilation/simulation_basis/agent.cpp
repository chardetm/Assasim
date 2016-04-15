/**
 * \file agent.cpp
 * \brief Implements methods of class Agent.
 */

#include <vector>
#include <unordered_set>
#include <mpi.h>

#include "types.hpp"
#include "agent_handler.hpp"
#include "master.hpp"
#include "agent.hpp"


Agent::Agent(AgentId id, AgentType type, MasterId master_id, Master& master) :
	id_{id}, type_{type}, master_id_{master_id}, structure_{nullptr}
{
	master_ = &master;
}


Agent::~Agent() {
	if (structure_ != nullptr) {
		free(structure_);
	}
}


bool Agent::DoesAgentExist(AgentId id, const AgentType type) {
	return master_->DoesAgentExist(id, type);
}


const std::set<AgentId>& Agent::GetAgentsOfType(const AgentType type) {
	return master_->GetAgentsOfType(type);
}


AgentId Agent::AgentIdTypeBound(const AgentType type) {
	return master_->AgentIdTypeBound(type);
}


void Agent::SendMessage(std::unique_ptr<Interaction> &&inter) {
	master_->PushInteraction(std::move(inter));
}


Time Agent::TimeStep() {
	return master_->TimeStep();
}


void* Agent::AskAttribute(Attribute attr, AgentId recipient_id, AgentType recipient_type) {
	return master_->GetAttribute(attr, recipient_id, recipient_type);
}


void* Agent::AskConstant(std::string constant) {
	return master_->GetConstant(constant);
}
