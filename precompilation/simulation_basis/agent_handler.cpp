/**
 * \file agent_handler.cpp
 * \brief Impelments the methods of agent handlers.
 */

#include "types.hpp"
#include "master.hpp"
#include "interaction.hpp"
#include "agent.hpp"
#include "agent_handler.hpp"


AgentHandler::AgentHandler(MasterId master_id, Master& master) : master_id{master_id} {
	this->master = &master;
}


Agent* AgentHandler::AddAgent(std::unique_ptr<Agent> &&agent) {
	AgentId agent_local_id = agent->id_;
	AgentType agent_type = agent->type_;
	agents.insert(std::make_pair(std::make_pair(agent_local_id, agent_type), std::forward<std::unique_ptr<Agent>>(agent)));
	return agents.at({agent_local_id, agent_type}).get();
}


void AgentHandler::RunBehaviors() {
	for (auto& agent : agents) {
		agent.second->Behavior();
		agent.second->ResetMessages();
		agent.second->CheckModifiedCriticalAttributes();
	}
}


void AgentHandler::UpdateAllPublicAttributes() {
	AgentId id;
	AgentType type;
	for (auto& agent : agents) {
		agent.second->CopyPublicAttributes(master->AgentPublicStructPointer(agent.second->id_, agent.second->type_));
		/* Only critical attributes which changed during the previous Behavior
		 * are updated.                                                       */
		for (auto& attr : agent.second->updated_critical_attributes_) {
			id = agent.second->id_;
			type = agent.second->type_;
			master->UpdateCriticalAttribute(attr, id, type, agent.second->GetPointerToAttribute(attr));
		}
		agent.second->updated_critical_attributes_.clear();
	}
}


void AgentHandler::DeleteAgent(AgentId id, AgentType type) {
	agents.erase({id, type});
}


void AgentHandler::GetJsonNodes(std::vector<ubjson::Value> &local_agents_by_types) {
	ubjson::Value agent_json;
	for (auto& agent : agents) {
		agent_json = agent.second->GetJsonNode();
		local_agents_by_types.at(agent.second->type_).push_back(std::move(agent_json));
	}
}
