/**
 * \file agent.hpp
 * \brief Defines what an agent should be and the functions that are shared by
 *        all agents.
 */

#ifndef AGENT_HPP_
#define AGENT_HPP_

#include <vector>
#include <set>
#include <unordered_set>
#include <memory>
#include <mpi.h>

#include "types.hpp"
#include "interaction.hpp"
#include "libs/ubjsoncpp/include/value.hpp"
#define $critical

/**
 * \struct AgentStruct
 * \brief Prototype of the structures used to migrate agents between masters
 *        using MPI, containing the first fields that are needed.
 *
 * \details The agent id and types must be explicitly written, and they will be
 * followed in each specific agent structure by a structure containing the whole
 * set of the attributes of the agent (private, public and critical).
 *
 * In order to be able to create their MPI_Datatype, this structures generated
 * in the precompilation step won't actually inherit AgentStruct, but will have
 * the same first fields id and type.
 */
struct AgentStruct {
	/// Local identifier of the agent.
	AgentId id;

	/// Type of the agent, which will help to decide the type of the returned
	/// agent.
	AgentType type;
};


/**
 * \class Agent
 *
 * \brief Agent is the the class which each agent class defined by the user
 *        will inherit after the precompilation step.
 *
 * \details This class indicates the basic requirements of any agent class.
 * It must implement a Behavior method, returning void, which can modify its own
 * attributes.
 *
 * Besides, it must have other methods that are not part of the model but can
 * depend on it (and thus are generated in the precompilation step): one to
 * send interactions, one to receive them, one to manage public attributes,
 * and so on.
 *
 * An agent is uniquely identified by its type and the id which characterizes it
 * among other agents of this type.
 *
 * Finally, an agent has hidden attributes which can be used in the model but
 * can't be manipulated or changed by it: relevant information about its master,
 * its neighborhood, and the interactions he received that are stored in a
 * queue.
 *
 * \todo Define and write UpdateEnvironment.
 */
class Agent {
	friend class AgentHandler;
	friend class Master;

public:

	/**
	 * \fn Agent(AgentId id, AgentType type, MasterId master_id, Master& master)
	 * \brief Constructor of an agent.
	 * \param id Local identifier of the future agent.
	 * \param type Type identifier of the future agent.
	 * \param master_id Identifier of the master which will hold the future
	 *        agent.
	 * \param master Reference to the master which will hold the future agent.
	 */
	Agent(AgentId id, AgentType type, MasterId master_id, Master& master);

	/**
	 * \fn ~Agent()
	 * \brief Destructor of an agent.
	 * \details Frees the field structure_ if needed.
	 */
	~Agent();

	/**
	 * \fn virtual void* GetPointerToAttribute(Attribute attr)
	 * \brief Returns the pointer to a given attribute of the agent.
	 * \param attr Attribute identifier.
	 * \return If attr is a valid attribute of the agent, the pointer to this
	 *         attribute; otherwise, nullptr.
	 * \remark
	 *   - Generated in the precompilation step.
	 *   - Different for each agent type.
	 */
	virtual void* GetPointerToAttribute(Attribute attr) = 0;

	/**
	 * \fn virtual ubjson::Value GetJsonNode()
	 * \brief Exports a structure representing the json representation of the
	 *        agent.
	 * \returns The ubjson::Value representing all attributes of the agent.
	 * \remark
	 *   - Generated in the precompilation step.
	 *   - Different for each agent type.
	 */
	virtual ubjson::Value GetJsonNode() = 0;

	/**
	 * \fn static std::unique_ptr<Agent> FromStruct(void *s, MasterId master_id, Master &master)
	 * \brief Builds and returns the agent represented by the structure given
	 *        as input.
	 * \param s Pointer to the structure representing an agent.
	 * \param master_id Identifier of the master which will handle the agent.
	 * \param master Reference to the master which will handle the agent.
	 * \return The unique pointer towards the agent represented by s.
	 * \pre The structure pointed by s must (virtually) inherit AgentStruct.
	 * \details Takes as input the structure representing an agent as well as
	 * infos about its master, creates the corresponding agent and returns its
	 * unique_ptr.
	 * \remark Generated in the precompilation step.
	 * \see AgentStruct
	 */
	static std::unique_ptr<Agent> FromStruct(void *s, MasterId master_id, Master &master);

protected:

	/// Identifier of the agent among all agents of the same type.
	AgentId id_;

	/// Type identifier of the agent (different for each class of agent).
	AgentType type_;

	/// Identifier of the agent's master.
	MasterId master_id_;

	/// Pointer to the agent's master.
	Master* master_;

	/// Stores the ids of agents that are in the neighborhood of this agent.
	std::unordered_set<AgentGlobalId> neighborhood_;

	/**
	 * At the end of an execution of CheckModifiedCriticalAttributes, contains
	 * the list of critical attributes that were modified during this execution.
	 */
	std::vector<Attribute> updated_critical_attributes_;

	/**
	 * Pointer to the data structure representing the agent class (used to
	 * send it with MPI), which (virtually) inherits AgentStruct.
	 * Only valid after the execution of CreateStruct.
	 * \see AgentStruct
	 */
	void* structure_;

	// TODO: After the migrations, changes the neighborhood of the agent
	void UpdateEnvironement();

	/**
	 * \fn Time TimeStep()
	 * \brief Returns the current time step.
	 * \return The current time step.
	 * \details The value of the current time step is stored in the agent's
	 * master, so this method accesses it.
	 */
	Time TimeStep();

	/**
	 * \fn bool DoesAgentExist(AgentId id, AgentType type)
	 * \brief Indicates if an agent characterized by its identifiers exists in
	 *        the simuation.
	 * \param id Local identifier of an agent.
	 * \param id Type identifier of an agent.
	 * \return true iff the agent of identifier id of type 'type' exists in the
	 *         current simulation.
	 */
	bool DoesAgentExist(const AgentId id, const AgentType type);

	/**
	 * \fn const std::set<AgentId>& GetAgentsOfType(AgentType type)
	 * \brief Gives the set of agents of a given type.
	 * \param type Type indentfier of agents.
	 * \return The const set containing the local identifiers of the input type.
	 */
	const std::set<AgentId>& GetAgentsOfType(AgentType type);

	/**
	 * \fn AgentId AgentIdTypeBound(AgentType)
	 * \brief Gives the maximal agent identifier + 1 of all agents of a given
	 *        type.
	 * \param type Type identifier of an agent.
	 * \return Max(identifiers of agents of type 'type') + 1
	 */
	AgentId AgentIdTypeBound(const AgentType type);

	/**
	 * \fn SendMessage(std::unique_ptr<Interaction> &&inter)
	 * \brief Notifies its master that this agent sends the interaction inter.
	 * \param inter Double reference to the unique_ptr of the sent interaction.
	 * \post The unique_ptr of the interaction is not valid after the execution
	 *       of this method anymore.
	 * \warning inter must not be used after the execution of this function.
	 */
	void SendMessage(std::unique_ptr<Interaction> &&inter);

	/**
	 * \fn void* AskAttribute(Attribute attr, AgentId recipient_id, AgentType recipient_type)
	 * \brief Computes a public attribute request.
	 * \param attr Attribute identifier of the requested attribute.
	 * \param recipient_id Local identifier of the agent whose attribute attr
	 *        is requested.
	 * \param recipient_type Type identifier of the agent whose attribute attr
	 *        is requested.
	 * \return Pointer to the memory location where the value of the requested
	 *         attribute is stored.
	 * \note Throws an AgentNotFound exception if the recipient agent does not
	 *       exist.
	 * \warning The value pointed by the returned pointer must not be modified.
	 */
	void* AskAttribute(Attribute attr, AgentId recipient_id, AgentType recipient_type);

	/**
	 * \fn void* AskConstant(std::string constant)
	 * \brief Gives the pointer to a constant of the simulation.
	 * \param constant Name of the constant.
	 * \return Pointer to the memory location where the value of constant is
	 *         stored.
	 * \warning The value pointed by the returned pointer must not be modified.
	 */
	void* AskConstant(std::string constant);

	/**
	 * \fn virtual void Behavior()
	 * \brief Main method of an agent, part of the model.
	 * \remark
	 *   - Transformed from the code of the user in the precompilation step.
	 *   - Different for each agent type.
	 */
	virtual void Behavior() = 0;

	/**
	 * \fn virtual void ResetMessages()
	 * \brief Deletes the interactions and errors received by the agent in the
	 *        previous time step.
	 * \remark
	 *   - Generated in the precompilation step.
	 *   - The code of this method is the same for all agents.
	 */
	virtual void ResetMessages() = 0;

	/**
	 * \fn virtual void ReceiveMessage(std::unique_ptr<Interaction> &inter)
	 * \brief Receives an interaction from its master and stores it.
	 * \param inter Reference to a unique_ptr to the interaction to receive.
	 * \warning inter must not be used after the execution of this function.
	 * \remark
	 *   - Generated in the precompilation step.
	 *   - The code of this method is the same for all agents.
	 */
	virtual void ReceiveMessage(std::unique_ptr<Interaction> &inter) = 0;

	/**
	 * \fn virtual void SetAttributeValue(Attribute attr, void* location)
	 * \brief Changes the value of an attribute of this agent.
	 * \param attr Attribute identifier of the attribute to modify.
	 * \param location Pointer to the memory location where the new value of the
	 *        attribute is stored.
	 * \remark
	 *   - Generated in the precompilation step.
	 *   - Different for each agent type.
	 */
	virtual void SetAttributeValue(Attribute attr, void* location) = 0;

	/**
	 * \fn virtual void CheckModifiedCriticalAttributes()
	 * \brief Fills updated_critical_attributes_ with the critical attributes
	 *        which were modified during the previous execution of Behavior.
	 * \remark
	 *   - Generated in the precompilation step.
	 *   - Different for each agent type.
	 */
	virtual void CheckModifiedCriticalAttributes() = 0;

	/**
	 * \fn virtual void CopyPublicAttributes(void *begin)
	 * \brief Copies the structure of all public (non critical) attributes of
	 *        the agent in the given memory location.
	 * \param begin Pointer to the beginning of the part of the window where the
	 *        public non critical attributes of the agent are stored.
	 * \remark
	 *   - Generated in the precompilation step.
	 *   - Different for each agent type.
	 */
	virtual void CopyPublicAttributes(void *begin) = 0;

	/**
	 * \fn virtual void CopyCriticalAttributes(void *begin)
	 * \brief Copies the structure of all critical attributes of the agent in
	 *        the given memory location.
	 * \param begin Pointer to the beginning of the part of the window where the
	 *        critical attributes of the agent are stored.
	 * \remark
	 *   - Generated in the precompilation step.
	 *   - Different for each agent type.
	 */
	virtual void CopyCriticalAttributes(void *begin) = 0;

	/**
	 * \fn virtual void CreateStruct()
	 * \brief Creates the structure representing this agent and makes
	 *        structure_ point towards it.
	 * \remark
	 *   - Generated in the precompilation step.
	 *   - Different for each agent type.
	 */
	virtual void CreateStruct() = 0;

};

#endif
