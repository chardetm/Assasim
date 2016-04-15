/**
 * \file master.hpp
 * \brief Defines functions and routines of the masters.
 */

#ifndef MASTER_HPP_
#define MASTER_HPP_

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <limits>
#include <thread>
#include <mpi.h>

#include "types.hpp"
#include "interaction.hpp"
#include "agent.hpp"


/**
 * \class Master
 *
 * \brief Master is the the class that manages a group of agents on a physical
 *        cluster.
 *
 * \details An instance of Master organizes the execution of the simulation for
 * a group of agents that are distributed among agent handlers (one group on an
 * agent handler being handled by a single thread) and takes care of sending and
 * receiving interactions to and from other agents that may be held by other
 * masters using MPI.
 *
 * The time step is described in RunTimeStep:
 *   - all interactions received by the master during the previous time step are
 *     distributed to the agents their are destined to;
 *   - the master orders the execution of the Behavior methods of all its agents;
 *   - the master orders and coordinates the update of public and critical
 *     attributes;
 *   - the master handles the migration, death and birth of agents;
 *   - finally the master sends to the right masters the interactions that were
 *     sent by the agents in their Behavior.
 *
 * During the Behavior methods execution, each agent can ask for a public or a
 * critical attribute of another agent, not necessarily held by this master.
 * Critical attributes of all agents are stored directly in all masters, and the
 * agents access it through GetCriticalAttribute. Non critical attributes may
 * be stored in another master; in that case, the method GetPublicAttribute
 * uses one-sided communication primitives of MPI thanks to the window
 * public_window, to access the location of this public attribute on another
 * physical machine without interrupting the execution of the master of this
 * machine.
 *
 * Most of the attributes of Master are generated in the precompilation step,
 * such as the MPI_Datatype's or the sizes of the attributes.
 *
 * In order to synchronize masters, we assume that master 0 is created and
 * handled on the root process. The function caracterized as "control method"
 * are functions where master 0 sends orders to other masters through a
 * broadcast, which they will receive while they are in WaitOrderFromRoot.
 *
 * \warning The creation and operation on masters must be handled between calls
 *          to MPI_Init_thread and MPI_Finalize.
 *
 * \todo TODO Implement AddUserAgents.
 * \todo TODO Migration, birth and death of agents.
 * \todo TODO Decide when an agent should migrate.
 * \todo TODO Define and implement environments.
 * \todo TODO Check the correctness of the code on several platforms
 *       (especially Windows).
 */
class Master {

	/**
	 * \enum Order
	 * \brief Lists all possible orders that can be sent to a master during the
	 *        execution of its method WaitOrder.
	 */
	enum class Order {
		/// Order used to completely stop the simulation and delete the masters.
		KILL_SIMULATION,

		/// Order used to run the simulation for some number of steps.
		RUN_SIMULATION,

		/// Order used to modify the number of steps in RunSimulation.
		CHANGE_PERIOD,

		/// Order used to warn that agents will be added to the simulation.
		ADD_AGENTS,

		/// Order used to modify some agent's attribute (either public or
		/// private).
		MODIFY_ATTRIBUTE,

		/// Order used to specify that master 0 should gather relevant infos
		/// about the simulation and export them.
		EXPORT_SIMULATION,

		/// Order used to pause the simulation.
		IDLE
	};

public:

	/**
	 * \fn Master (MasterId id_, MasterId nb_masters_, int nb_threads, std::vector<void*> &initial_agents)
	 * \brief Constructor of Master, which initializes the parameters of the
	 *        simulation as well as randomness.
	 * \param id Identifier and MPI rank of the created master.
	 * \param nb_masters Number of masters used in the simulation.
	 * \param initial_agents Reference to the vector of pointers to AgentStructs
	 *        representing the initial agents.
	 *
	 * \attention initial_agents is only useful for master 0; all other should
	 * receive an empty vector.
	 *
	 * \attention If an agent has been initialized with a non trivial non
	 * sendable parameter, then, if it is moved to another master, this
	 * parameter will be set to its default value (empty vector, for example).
	 */
	Master (MasterId id, MasterId nb_masters, int nb_threads, std::vector<void*> &initial_agents);

	/**
	 * \fn ~Master()
	 * \brief Destructor of Master. Frees the windows and the MPI objects used in
	 *        the master.
	 */
	~Master();

	/**
	 * \fn Time TimeStep()
	 * \brief Indicates the current time step.
	 * \return The value of the current time step.
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
	bool DoesAgentExist(AgentId id, AgentType type);

	/**
	 * \fn const std::unordered_set<AgentId>& GetAgentsOfType(AgentType type)
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
	AgentId AgentIdTypeBound(AgentType type);

	/**
	 * \fn void* AgentPublicStructPointer(AgentId id, AgentType type)
	 * \brief Computes the emplacement of the beginning of the set of public non
	 *        critical attributes of an agent stored in this master.
	 * \param id Local identifier of the input agent, held by this master.
	 * \param id Type identifier of the input agent, held by this master.
	 * \return Pointer to the memory location of the beginning of the set of
	 *         public non critical attributes of agent id.
	 */
	void* AgentPublicStructPointer(AgentId id, AgentType type);

	/**
	 * \fn size_t PublicTargetDisp(AgentGlobalId id, Attribute attr)
	 * \brief Computes the displacement from the public window start where the
	 *        some public non critical attribute of an agent.
	 * \param id Global identifier of an agent.
	 * \param attr Attribute identifier of a public non critical attribute of
	 *        agent id.
	 * \return The number of bytes between the beginning of public_window and
	 *         the beginning of memory location where attr is stored.
	 */
	size_t PublicTargetDisp(AgentGlobalId id, Attribute attr);

	/**
	 * \fn void* GetConstant(std::string constant)
	 * \brief Gives the pointer to a constant of the simulation.
	 * \param constant Name of the constant.
	 * \return Pointer to the memory location where the value of constant is
	 *         stored.
	 * \warning The value pointed by the returned pointer must not be modified.
	 */
	void* GetConstant(std::string constant);

	/**
	 * \fn void* GetAttribute(Attribute attr, AgentId recipient_id, AgentType recipient_type)
	 * \brief Computes a public attribute request from an agent.
	 * \param attr Attribute identifier of the requested attribute.
	 * \param recipient_id Local identifier of the agent which holds the
	 *        requested attribute.
	 * \param recipient_type Type identifier of the agent which holds the
	 *        requested attribute.
	 * \return The pointer to the memory location where the value of the
	 *         requested attribute is stored if the input agent exists.
	 * \details Returns the memory location where the value of public attribute
	 * attr of agent (recipient_id, recipient_type) is stored. If attr is
	 * critical, it checks the data base of this master in critical_window;
	 * otherwise it accesses the public window of the recipient's master by an
	 * RDMA operation.
	 * \note Throws an AgentNotFound exception if the recipient agent does not
	 *       exist.
	 * \warning The value pointed by the returned pointer must not be modified.
	 */
	void* GetAttribute(Attribute attr, AgentId recipient_id, AgentType recipient_type);

	/**
	 * \fn void UpdateCriticalAttribute(Attribute attr, AgentId agent_id, AgentType agent_type, void *location)
	 * \brief Updates in all critical windows of all masters the attribute attr.
	 * \param attr Attribute identifier of the attribute to update.
	 * \param agent_id Local identifier of the agent which owns the attribute to
	 *        update.
	 * \param agent_type Type identifier of the agent which owns the attribute to
	 *        update.
	 * \param location Pointer to the memory location where attribute attr is
	 *        stored.
	 */
	void UpdateCriticalAttribute(Attribute attr, AgentId agent_id, AgentType agent_type, void *location);

	/**
	 * \fn void PushInteraction(std::unique_ptr<Interaction> &&inter)
	 * \brief Receives an interaction to send from on the agents of the master.
	 * \param inter Double reference to the unique_ptr of the sent interaction.
	 * \post The unique_ptr of the interaction is not valid after the execution
	 *       of this method anymore.
	 * \details Ignores the interaction if its recipient does not exist.
	 * \warning inter must not be used after the execution of this function.
	 */
	void PushInteraction(std::unique_ptr<Interaction> &&inter);

	/**
	 * \fn void RunSimulation()
	 * \brief Runs nb_steps time steps.
	 * \note RunSimulation is a control method.
	 * \warning This function must only be externally called on master 0, while
	 *          other masters are in WaitOrderFromRoot.
	 */
	void RunSimulation();

	/**
	 * \fn void ChangePeriod(Time new_period)
	 * \brief Modifies the period_ of master 0 to new_period, and sends it to
	 *        the other masters
	 * \param new_period The new period to register on master 0.
	 * \note The argument new_period is only relevant for master 0.
	 * \note RunSimulation is a control method.
	 * \warning This function must only be externally called on master 0, while
	 *          other masters are in WaitOrderFromRoot.
	 */
	void ChangePeriod(Time new_period = 0);

	/**
	 * \fn void AddUserAgents(std::vector<void*> &new_agents)
	 * \brief Orders the other masters to add some agents to the simulation.
	 * \param new_agents Reference to a vector of pointers to the agents to add.
	 * \note RunSimulation is a control method.
	 * \note The argument new_agents is only relevant for master 0.
	 * \warning This function must only be externally called on master 0, while
	 *          other masters are in WaitOrderFromRoot.
	 */
	void AddUserAgents(std::vector<void*> &new_agents);

	/**
	 * \fn void ModifyAttribute(Attribute attr, AgentId agent_id, AgentType agent_type, void* location)
	 * \brief Orders the simuation to modify some agent's attribute.
	 * \param attr On master 0, the attribute identifier of the attribute to
	 *        modify.
	 * \param agent_id Local identifier of the agent whose attribute attr must
	 *        be modified.
	 * \param agent_id Type identifier of the agent whose attribute attr must
	 *        be modified.
	 * \param location Pointer to the memory location where the new value of
	 *        attribute attr is stored.
	 * \note RunSimulation is a control method.
	 * \warning This function must only be externally called on master 0, while
	 *          other masters are in WaitOrderFromRoot.
	 * \warning The values of the arguments of this method must not be used on
	 *          masters that are not master 0.
	 */
	void ModifyAttribute(Attribute attr = 0, AgentId agent_id = 0, AgentType agent_type = 0, void* location = nullptr);

	/**
	 * \fn ubjson::Value ExportSimulation()
	 * \brief Handles the export of the simulation in a json format.
	 * \details Orders the other masters to gather all infos about their part of
	 * the simulation, and send them to master 0; then master 0 handles the
	 * export of data.
	 * \note RunSimulation is a control method.
	 * \remark The returned value is only significant for master 0.
	 * \warning This function must only be externally called on master 0, while
	 *          other masters are in WaitOrderFromRoot.
	 */
	ubjson::Value ExportSimulation();

	/**
	 * \fn void KillSimulation()
	 * \brief Orders the other masters that the simulation must be stopped and
	 *        that they can exit WaitOrderFromRoot.
	 * \warning This function must only be called on master 0, while other
	 *          masters are in WaitOrderFromRoot.
	 */
	void KillSimulation();

	/**
	 * \fn void WaitOrderFromRoot()
	 * \brief Method used to wait for an order from master 0 for all other
	 *        masters.
	 * \details It uses Busy waiting, and exits the method if the order
	 * KILL_SIMULATION is received.
	 * \attention This method does nothing if called on master 0.
	 */
	void WaitOrderFromRoot();

	/**
	 * \fn void ConvertOutputToInput(std::string in, std::string out)
	 * \brief Convert a binary Json file describing an output of the simulation
	 *        in a file that can be read by the initialization.
	 * \param in File to be converted.
	 * \param out File where the conversion has to be stored.
	 */
	void ConvertOutputToInput(std::string in, std::string out);

protected:

private:

	/**
	 * Index of the current time step.
	 */
	Time step_;

	/**
	 * Order currently executed.
	 */
	Order order_;

	/**
	 * Number of time steps executed in a run of RunSimulation.
	 */
	Time period_;

	/**
	 * Identifier of the master and its rank in MasterComm.
	 */
	MasterId id_;

	/**
	 * Communicator for all the masters.
	 */
	MPI_Comm MasterComm_;

	/**
	 * Total number of masters.
	 */
	MasterId nb_masters_;

	/**
	 * Number of agent types.
	 */
	AgentType nb_types_;

	/**
	 * Vector associating to each agent type the maximal id +1 for an existing
	 * agent of this type (0 if there is none).
	 */
	std::vector<AgentId> maximal_ids_;

	/**
	 * Number of interaction types.
	 */
	InteractionType nb_interactions_;

	/**
	 * Map associating to a constant name the memory location of its value.
	 */
	std::unordered_map<std::string, void*> constants_;

	/**
	 * Maximum size of an existing InteractionStruct.
	 */
	size_t max_interaction_size_;

	/**
	 * Maximum size of an existing AgentStruct.
	 */
	size_t max_agent_size_;

	/**
	 * Window used for the storage of public (non critical) attributes.
	 */
	MPI_Win public_window_;

	/**
	 * Window used for the storage of critical attributes.
	 */
	MPI_Win critical_window_;

	/**
	 * Pointer pointing towards the beginning of the content of public_window.
	 */
	void* begin_public_window_;

	/**
	 * Pointer pointing towards the beginning of the content of critical_window.
	 */
	void* begin_critical_window_;

	/**
	 * Window Description used for the storage of public (non critical) attributes.
	 */
	WindowDescription CriticalWindowDescription;

	/**
	 * Window Description used for the storage of critical attributes.
	 */
	std::vector<WindowDescription> PublicWindowsDescription;

	/**
	 * Map of the MPI types of all existing AgentStruct's.
	 */
	std::unordered_map<AgentType, MPI_Datatype> agents_MPI_types_;

	/**
	 * Map of the MPI types of the structures representing the sets of critical
	 * attributes of all types. If an agent type has no critical attribute,
	 * this map does not contain the corresponding entry.
	 */
	std::unordered_map<AgentType, MPI_Datatype> critical_structs_MPI_types_;

	/**
	 * Map of the MPI types of all attributes for all types of interactions.
	 */
	std::unordered_map<InteractionType, MPI_Datatype> interactions_MPI_types_;

	/**
	 * Vector associating to an agent type the set of agents local identifiers
	 * of this type.
	 */
	std::vector<std::set<AgentId>> agent_ids_by_types_;

	/**
	 * Map between all the agent IDs and their associated Master.
	 */
	std::unordered_map<AgentGlobalId, MasterId> masters_;

	/**
	 * Map between all the agent IDs and their ``memory location''.
	 * It could've been a simple map <AgentGlobalId, Agent&> agents, still,
	 * removing an agent from the Agent Handler vector agents_ would have
	 * invalidated the pointer here.
	 */
	std::unordered_map<AgentGlobalId, Agent*> agents_;

	/**
	 * Stores pairs (agent_global_id, attribute where attribute is a critical)
	 * attribute of agent agent_global_id.
	 */
	CriticalAttributes critical_attributes_;

	/**
	 * Stores the types of the agents that are not sendable because one of their
	 * private attributes is not sendable (complex data structure).
	 */
	std::unordered_set<AgentType> non_sendable_agent_types_;

	/**
	 * Vector of agent handlers managed by the Master.
	 */
	std::vector<AgentHandler> agent_handlers_;

	/**
	 * Map of the sizes of all (public and private) sendable attributes for all
	 * types of agents.
	 */
	AttributesSizes attributes_sizes_;

	/**
	 * Map of the MPI types of all (public and private) sendable attributes
	 * for all types of agents.
	 */
	AttributesMPITypes attributes_MPI_types_;

	/**
	 * Map of the offsets of public (non critical) attributes in the structure
	 * of the public (non critical) attributes of a type of agents, for all
	 * types of agents.
	 */
	AttributesOffsets public_attributes_offsets_;

	/**
	 * Gives for an agent the offset of its structure of public (non critical)
	 * attributes in the public window of its master.
	 */
	std::unordered_map<AgentGlobalId, size_t> public_agents_offsets_;

	/**
	 * Map of the sizes of the whole structure of public (non critical)
	 * attributes for all types of agents.
	 */
	std::unordered_map<AgentType, size_t> public_attributes_struct_sizes_;

	/**
	 * Map of the offsets of critical attributes of an agent in the structure of
	 * its critical attribites, for all types of agents.
	 */
	AttributesOffsets critical_attributes_offsets_;

	/**
	 * Map which gives for an agent the offset of its structure of critical
	 * attributes in the window critical_window (same offset on all master).
	 */
	std::unordered_map<AgentGlobalId, size_t> critical_agents_offsets_;

	/**
	 * Map of the sizes of the whole structire of critical attributes of an
	 * agent, for all types of agents.
	 */
	std::unordered_map<AgentType, size_t> critical_attributes_struct_sizes_;

	/**
	 * Associates to each agent type its name.
	 * \attention This structure is empty in all masters but master 0.
	 */
	std::unordered_map<AgentType, AgentName> agent_type_to_string_;

	/**
	 * Associates to each attribute its name.
	 * \attention This structure is empty in all masters but master 0.
	 */
	AttributesNames attribute_to_string_;

	/**
	 * Associates to a string the agent type it represents.
	 * \attention This structure is empty in all masters but master 0.
	 */
	std::unordered_map<AgentName, AgentType> string_to_agent_type_;

	/**
	 * Associate to a string the attribute it represents.
	 * \attention This structure is empty in all masters but master 0.
	 */
	AttributesIds string_to_attribute_;

	/**
	 * Interactions received in a time step.
	 */
	InteractionContainer received_interactions_;

	/**
	 * Interactions that are asked to be sent by the agents of this master.
	 */
	InteractionMatrix interactions_to_send_;

	/**
	 * Container used in SendReceiveInteractions (we need to keep it for
	 * performance issues).
	 */
	utils::fixed_size_multibuffer<InteractionStruct> interactions_buffer_;

	/**
	 * Map used to remember which public non critical attributes were already
	 * asked by an agent of this master and, if so, associates to it its memory
	 * location.
	 */
	ReceivedAttributesThreadSafe received_public_attributes_;

	/**
	 * Memory location where the received public non critical attributes are
	 * stored.
	 */
	utils::custom_heap stored_public_attributes_;

	/**
	 * \fn AgentGlobalId LocalToGlobalId(AgentId id, AgentType type)
	 * \brief Computes the global id of an agent from its local identifiers.
	 * \param id Local identifier of the agent given as input.
	 * \param type Type identifier of the agent given as input.
	 * \return The global identifier of the agent characterized by (id, type).
	 */
	AgentGlobalId LocalToGlobalId(AgentId id, AgentType type);

	/**
	 * \fn AgentId GlobalToLocalId(AgentGlobalId id)
	 * \brief Computes the lcoal identifier of an agent from its global
	 *        identifier.
	 * \param id Global identifier of an agent.
	 * \return The local identifier of the agent given as input.
	 */
	AgentId GlobalToLocalId(AgentGlobalId id);

	/**
	 * \fn AgentType GlobalToLocalType(AgentGlobalId id)
	 * \brief Computes the type of an agent from its global identifier.
	 * \param id Global identifier of an agent.
	 * \return The type identifier of the agent given as input.
	 */
	AgentType GlobalToLocalType(AgentGlobalId id);

	/**
	 * \fn size_t PublicWindowsSize()
	 * \brief Gives the size allocated to every public window.
	 * \return The size allocated to the local public window.
	 * \warning This method only works if all public windows have the same size.
	 */
	size_t PublicWindowsSize();

	/**
	 * \fn size_t CriticalWindowsSize()
	 * \brief Gives the size allocated to every critical window.
	 * \return The size allocated to the local critical window.
	 */
	size_t CriticalWindowsSize();

	/**
	 * \fn bool IsCritical(Attribute attr, AgentType type)
	 * \brief Indicates if an attribute of a given agent type is critical.
	 * \param attr Attribute identifier.
	 * \param type Type identifier of an agent.
	 * \return true iff attribute attr of agent type 'type' is critical.
	 */
	bool IsCritical(Attribute attr, AgentType type);

	/**
	 * \fn bool IsAgentSendable(AgentType type)
	 * \brief Indicates if an agent of type 'type' is sendable with MPI, i.e. can
	 *        be migrated.
	 * \param type Type identifier of an agent.
	 * \return true iff an agent of type 'type' is sendable.
	 */
	bool IsAgentSendable(AgentType type);

	/**
	 * \fn bool IsAttributeSendable(Attribute attr, AgentType type)
	 * \brief Indicates if an attribute is sendable with MPI.
	 * \param attr Attribute identifier.
	 * \param type Type identifier of an agent.
	 * \return true iff attribute attr of an agent of type 'type' is sendable.
	 */
	bool IsAttributeSendable(Attribute attr, AgentType type);

	/**
	 * \fn bool HasReceivedAttribute(Attribute attr, AgentGlobalId id, void* location)
	 * \brief Checks if a public attribute request has already been fulfilled.
	 * \param attr Attribute identifier of the requested attribute.
	 * \param id Global identifier of the agent whose attribute attr is
	 *        requested.
	 * \param location At the end of the execution, reference to the pointer to
	 *        the requested attribute value.
	 * \return true iff public non critical attribute attr of agent id has
	 *         already been requested in this master.
	 * \details Returns true iff public non critical attribute attr of agent id
	 * has already been asked by an agent held by this master. If so, places the
	 * pointer to the result in location.
	 */
	bool HasReceivedAttribute(Attribute attr, AgentGlobalId id, void* &location);

	/**
	 * \fn void InitializeAgents(std::vector<void*> &initial_agents)
	 * \brief Initialize all parameters about the initial agents in all masters.
	 * \param initial_agents Reference to the vector containing the pointers to
	 *        AgentStructs representing the initial agents of the simulation.
	 * \details Receives all the initial agents from master 0 (initially stored
	 * in initial_agents) and adds them in this master. Receives also the
	 * masters attribution of all agents. Finally, use the available data about
	 * the agents to initialize the windows.
	 * \attention initial_agents is only useful for master 0; all other should
	 *            receive an empty vector.
	 */
	void InitializeAgents(std::vector<void*> &initial_agents);

	/**
	 * \fn void InitializeWindows(std::vector<AgentGlobalId> &agent_ids)
	 * \brief Initializes all the parameters concerning MPI windows.
	 * \param agent_ids Reference to a vector containing all agent global
	 *        identifiers at the beginning of the simulation.
	 * \pre The contents of agent_ids must be the same in all masters.
	 * \remark All public windows are initialized to the same for performance
	 *         issues.
	 * \details Also initializes maximal_ids_ and agent_ids_by_types_.
	 */
	void InitializeWindows(std::vector<AgentGlobalId> &agent_ids);

	/**
	 * \fn void FillWindows(std::vector<AgentGlobalId> &agent_ids)
	 * \param agent_ids Reference to a vector containing all agent global
	 *        identifiers at the beginning of the simulation.
	 * \pre The contents of agent_ids must be the same in all masters.
	 * \details Fills the windows with the contents they should have (public /
	 * critical attributes of the agents), given the list of all agent ids
	 * agent_ids.
	 */
	void FillWindows(std::vector<AgentGlobalId> &agent_ids);

	/**
	 * \fn void AddAgent(AgentHandler &agent_handler, void *structure)
	 * \brief Creates an agent of the right type from the structure representing
	 *        it, and adds it to this master and to the agent handler
	 *        agent_handler.
	 * \param agent_handler Reference to the agent hander where the agent
	 *        represented by structure has to be added.
	 * \param structure Pointer to a structure representing an agent.
	 */
	void AddAgent(AgentHandler &agent_handler, void *structure);

	/**
	 * \fn void Synchronize()
	 * \brief Synchronizes all the masters and the agent handlers.
	 */
	void Synchronize();

	/**
	 * \fn void DistributeReceivedInteractions()
	 * \brief Distributes to the agents the interactions received in a time
	 *        step.
	 */
	void DistributeReceivedInteractions();

	/**
	 * \fn void RunBehaviors()
	 * \brief Executes the behaviors of all the agents held in this master.
	 * \details This method executes in parallel the behaviors af agents that
	 * are not held by the same agent handler.
	 */
	void RunBehaviors();

	/**
	 * \fn void* GetPublicAttribute(Attribute attr, AgentGlobalId recipient)
	 * \brief Processes a public non critical attribute request from an agent
	 *        managed by this master.
	 * \param attr Attribute identifier of the requested attribute.
	 * \param recipient Global identifier of the agent whose attribute attr is
	 *        requested.
	 * \return A pointer to the memory location where the value of the requested
	 *         attribute is stored.
	 * \details The attribute attr is asked to the agent recipient, and is
	 * copied in the memory location which the returned pointer points to.
	 * \warning The value pointed by the returned pointer must not be modified.
	 */
	void* GetPublicAttribute(Attribute attr, AgentGlobalId recipient);

	/**
	 * \fn void* GetCriticalAttribute(Attribute attr, AgentGlobalId recipient)
	 * \brief Processes a critical attribute request from an agent managed by
	 *        this master.
	 * \param attr Attribute identifier of the requested attribute.
	 * \param recipient Global identifier of the agent whose attribute attr is
	 *        requested.
	 * \warning The value pointed by the returned pointer must not be modified.
	 */
	void* GetCriticalAttribute(Attribute attr, AgentGlobalId recipient);

	/**
	 * \fn void UpdateAllPublicAttributes()
	 * \brief Performs an update of all public attributes of the agents of this
	 *        master.
	 */
	void UpdateAllPublicAttributes();

	/**
	 * \fn void SendReceiveInteractions()
	 * \brief Sends all interactions emitted by the agents to the masters of
	 * their recipients and receives all interactions to be read by this
	 * master's agents.
	 */
	void SendReceiveInteractions();

	/**
	 * \fn void MetaEvolution()
	 * \brief Decides the agents that should migrate, and performs agent deaths,
	 *        births and migrations.
	 */
	void MetaEvolution();

	/**
	 * \fn void RunTimeStep()
	 * \brief Organizes and runs a time step.
	 */
	void RunTimeStep();

	/**
	 * Contains the agents that we need to create at each time step.
	 */
	utils::thread_safe_vector<std::pair<AgentType, void *>> AgentsToCreate;

	/**
	 * Contains the agents that we need to delete at each time step.
	 */
	utils::thread_safe_vector<AgentId> AgentsToDelete;

	/**
	 * Contains the MetaEvolutions that we will send to other Masters.
	 */
	std::vector<MetaEvolutionDescription> LocalMetaEvolutionDescriptions;

	/**
	 * Contains the MetaEvolutions that we recieved from all the other
	 * Masters.
	 */
	std::vector<MetaEvolutionDescription> GlobalMetaEvolutionDescriptions;

};

#endif
