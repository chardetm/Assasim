/**
 * \file agent_handler.hpp
 * \brief Defines the agent handlers.
 */

#ifndef AGENT_HANDLER_HPP_
#define AGENT_HANDLER_HPP_

#include "types.hpp"
#include "libs/ubjsoncpp/include/value.hpp"


/**
 * \class AgentHandler
 *
 * \brief AgentHandler is the entity that manages all of the agents of a thread.
 *
 * \details An AgentHandler essentially takes care of the execution of the
 * Behavior methods of all the agents it manages. It is able to add or erase an
 * agent of its scope. Moreover, it handles the execution of other methods that
 * must be executed by all agents and that can be all executed in parallel, like
 * UpdatePublicAttributes.
 */
class AgentHandler {

public:

	/**
	 * \fn AgentHandler(MasterId master_id, Master& master)
	 * \brief Creates an agent handler.
	 * \param master_id Id of the master which the created agent handler belongs
	 *        to.
	 * \param master Reference to the master which the created agent handler
	 *        belongs to.
	 */
	AgentHandler(MasterId master_id, Master& master);

	/// Identifier of this agent handler's master.
	MasterId master_id;

	/// Pointer to this agent handler's master.
	Master* master;

	/// Agents held by this agent handler.
	AgentContainer agents;

	/**
	 * \fn void AddAgent(std::unique_ptr<Agent> &&agent)
	 * \brief Adds an agent to this agent handler and releases its unique_ptr.
	 * \param agent Double reference to the unique_ptr of the added agent.
	 * \return The pointer to the added agent.
	 * \post The unique_ptr of the agent is released after the execution of this
	 *       method.
	 * \warning agent must not be used after the execution of this function.
	 */
	Agent* AddAgent(std::unique_ptr<Agent> &&agent);

	/**
	 * \fn void RunBehaviors()
	 * \brief Runs the Behaviors of all agents in this thread.
	 */
	void RunBehaviors();

	/**
	 * \fn void UpdateAllPublicAttributes()
	 * \brief Updates in this agent handler's master the values of the public
	 *        and critical attributes of all agents of this agent handler.
	 * \details For each agent in agents, copies the set of its public non
	 * critical attributes in the public window of its master, and informs its
	 * master of the values of the critical attributes which changed during the
	 * previous execution of Behavior.
	 */
	void UpdateAllPublicAttributes();

	/**
	 * \fn void DeleteAgent(AgentId id, AgentType type)
	 * \brief Deletes the agent represented by its local id and its type.
	 * \param id Local identifier of the agent to delete.
	 * \param type Type identifier of the agent to delete.
	 */
	void DeleteAgent(AgentId id, AgentType type);

	/**
	 * \fn void GetJsonNodes(std::vector<ubjson::Value> &local_agents_by_types)
	 * \brief Writes in result in the binary json format the content of all the
	 *        agents of this agent handler.
	 * \param local_agents_by_types Reference to the vector of values where
	 *        entry i is where data of agents of type i is stored.
	 */
	void GetJsonNodes(std::vector<ubjson::Value> &local_agents_by_types);

};

#endif
