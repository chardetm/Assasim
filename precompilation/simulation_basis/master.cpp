/**
 * \file master.cpp
 * \brief Implements functions and routines of the masters.
 */

#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <thread>
#include <ctime>
#include <cstdlib>
#include <mpi.h>

#include "types.hpp"
#include "master.hpp"
#include "agent.hpp"
#include "agent_handler.hpp"
#include "interaction.hpp"
#include "parameters_generation.hpp"
#include "heuristics.hpp"
#include "libs/ubjsoncpp/include/value.hpp"
#include "libs/ubjsoncpp/include/stream_reader.hpp"
#include "libs/ubjsoncpp/include/stream_writer.hpp"


// MPI Type of this structure
MPI_Datatype MetaEvolutionDescriptionMPIDatatype;


Master::Master (MasterId id, MasterId nb_masters, int nb_threads, std::vector<void*> &initial_agents) :

	step_{0}, order_{Order::IDLE}, period_{1}, id_{id}, nb_masters_{nb_masters}

{
	// Randomness initialization
	srand(time(NULL) + id_);

	// Initialization of the parameters of the model by the precompilation step
	nb_types_ = NbAgentTypes();
	nb_interactions_ = NbInteractionTypes();
	CreateAttributesMPIDatatypes(attributes_MPI_types_);
	max_agent_size_ = CreateAgentsMPIDatatypes(agents_MPI_types_, attributes_MPI_types_);
	CreateCriticalStructsMPIDatatypes(critical_structs_MPI_types_, attributes_MPI_types_);
	max_interaction_size_ = CreateInteractionsMPIDatatypes(interactions_MPI_types_);
	CreateAttributesSizes(attributes_sizes_);
	CreateNonSendableAgentTypes(non_sendable_agent_types_);
	CreatePublicAttributesOffsets(public_attributes_offsets_);
	CreatePublicStructSizes(public_attributes_struct_sizes_);
	CreateCriticalAttributesOffsets(critical_attributes_offsets_);
	CreateCriticalStructSizes(critical_attributes_struct_sizes_);
	CreateCriticalAttributes(critical_attributes_);

	CreateAgentsNamesRelation(agent_type_to_string_, string_to_agent_type_);
	CreateAttributesNamesRelation(attribute_to_string_, string_to_attribute_);

	// TODO: Uncomment once precompilation handled constant
	// GenerateConstants(constants_);

	// Initialization of the MPI Datatypes for the Meta Evolutions
	generateMPIDatatype(MetaEvolutionDescriptionMPIDatatype);

	// Creation of the agent handlers
	for (int k=0; k<nb_threads; k++) {
		agent_handlers_.emplace_back(id_, *this);
	}

	// Initialization of the queues
	interactions_to_send_ = InteractionMatrix(nb_masters_*nb_interactions_);
	interactions_buffer_ = utils::fixed_size_multibuffer<InteractionStruct>(max_interaction_size_);

	// Initialization of the master communicator
	MPI_Comm_split(MPI_COMM_WORLD, 0, id_, &MasterComm_);

	// Receive and adds agents
	InitializeAgents(initial_agents);

}

Master::~Master() {
	// Freeing the constants
	for (auto &c : constants_) {
		free(c.second);
	}

	// Freeing MPI objects
	int is_finalized;
	MPI_Finalized(&is_finalized);
	if (!is_finalized) {
		MPI_Win_free(&public_window_);
		MPI_Win_free(&critical_window_);
		MPI_Type_free(&MetaEvolutionDescriptionMPIDatatype);
		MPI_Comm_free(&MasterComm_);
	}
}


Time Master::TimeStep() {
	return step_;
}


bool Master::DoesAgentExist(AgentId id, AgentType type) {
	return agent_ids_by_types_.at(type).find(id) != agent_ids_by_types_.at(type).end();
}


const std::set<AgentId>& Master::GetAgentsOfType(AgentType type) {
	return agent_ids_by_types_.at(type);
}


AgentId Master::AgentIdTypeBound(AgentType type) {
	return maximal_ids_.at(type);
}


void* Master::AgentPublicStructPointer(AgentId id, AgentType type) {
	AgentGlobalId global_id = LocalToGlobalId(id, type);
	return static_cast<char*>(begin_public_window_) + public_agents_offsets_.at(global_id);
}


size_t Master::PublicTargetDisp(AgentGlobalId id, Attribute attr) {
	AgentType type = GlobalToLocalType(id);
	return public_agents_offsets_.at(id) + public_attributes_offsets_.at(std::make_pair(type, attr));
}


void* Master::GetConstant(std::string constant) {
	return constants_.at(constant);
}


void* Master::GetAttribute(Attribute attr, AgentId recipient_id, AgentType recipient_type) {
	AgentGlobalId id = LocalToGlobalId(recipient_id, recipient_type);
	if (!DoesAgentExist(recipient_id, recipient_type)) {
		throw AgentNotFound(recipient_id, agent_type_to_string_.at(recipient_type));
	} else if (IsCritical(attr, recipient_type)) {
		return GetCriticalAttribute(attr, id);
	} else {
		return GetPublicAttribute(attr, id);
	}
}


void Master::UpdateCriticalAttribute(Attribute attr, AgentId agent_id, AgentType agent_type, void* location) {
	AgentType type = GlobalToLocalType(agent_id);
	auto p = std::make_pair(type, attr);
	size_t target_disp = critical_agents_offsets_.at(agent_id) + critical_attributes_offsets_.at(p);
	MPI_Datatype attribute_type = attributes_MPI_types_.at(p);
	for (MasterId id=0; id<nb_masters_; id++) {
		MPI_Put(location, 1, attribute_type, id, target_disp, 1, attribute_type, critical_window_);
	}
}


void Master::PushInteraction(std::unique_ptr<Interaction> &&inter) {
	InteractionType type = inter->type_;
	AgentGlobalId recipient_id = LocalToGlobalId(inter->recipient_id_, inter->recipient_type_);
	if (DoesAgentExist(inter->recipient_id_, inter->recipient_type_)) {
		MasterId recipient_master = masters_.at(recipient_id);
		interactions_to_send_.at(recipient_master*nb_interactions_+type).push_back(std::move(inter));
	} else {
		std::cerr << "Warning: Agent " << inter->sender_id_ << " of type " << agent_type_to_string_.at(inter->sender_type_)
		          << " sent an interaction to an agent which does not exist." << std::endl
				  << "Agent " << inter->recipient_id_ << " of type " << agent_type_to_string_.at(inter->recipient_type_)
				  << " can not be found in the simulation. The interaction is ignored." << std::endl;
	}
}


void Master::RunSimulation() {
	// This method is a control method, so sends orders from master 0 to other
	// masters
	if (id_ == 0) {
		order_ = Order::RUN_SIMULATION;
		MPI_Bcast(&order_, 1, MPI_INT, 0, MasterComm_);
	}
	for (Time t=0; t<period_; t++) {
		RunTimeStep();
	}
}


void Master::ChangePeriod(Time new_period) {
	if (id_ == 0) {
		// This method is a control method, so sends orders from master 0 to other
		// masters
		order_ = Order::CHANGE_PERIOD;
		MPI_Bcast(&order_, 1, MPI_INT, 0, MasterComm_);
		// Changes the time period
		period_ = new_period;
	}
	// Receives the new period from master 0
	MPI_Bcast(&period_, 1, MPI_UINT64_T, 0, MasterComm_);
}


// TODO
void Master::AddUserAgents(std::vector<void*> &new_agents) {
	// This method is a control method, so sends orders from master 0 to other
	// masters
	if (id_ == 0) {
		order_ = Order::ADD_AGENTS;
		MPI_Bcast(&order_, 1, MPI_INT, 0, MasterComm_);
	}
	// TODO
}


void Master::ModifyAttribute(Attribute attr, AgentId agent_id, AgentType agent_type, void* location) {
	AgentGlobalId recipient_global_id = LocalToGlobalId(agent_id, agent_type);
	if (id_ == 0) {
		// First we check that the agent type agent_type exists
		if (agent_type >= nb_types_) {
			std::cerr << "The agent type " << agent_type << " does not exist." << std::endl;
			return;
		}
		// Then we check that this attribute is sendable
		if (!IsAttributeSendable(attr, agent_type)) {
			std::cerr << "This attribute cannot be modified: it is not sendable." << std::endl;
			return;
		}
		// Finally we check if the agent recipient_global_id exists
		if (!DoesAgentExist(agent_id, agent_type)) {
			std::cerr << "The agent " << agent_id << "of type" << agent_type_to_string_.at(agent_type)
					  << " does not exist." << std::endl;
			return;
		}
		// This method is a control method, so sends orders from master 0 to
		// other masters
		order_ = Order::MODIFY_ATTRIBUTE;
		MPI_Bcast(&order_, 1, MPI_INT, 0, MasterComm_);
	}
	// Sends the info to the other masters
	MPI_Bcast(&recipient_global_id, 1, MPI_UINT64_T, 0, MasterComm_);
	MasterId recipient_master = masters_.at(recipient_global_id);
	auto p = std::make_pair(GlobalToLocalType(recipient_global_id), attr);
	// If the recipient of this request is not in master 0, some communications
	// must be performed
	if (recipient_master != 0) {
		// Sends the attribute identifier and its new content to the concerned master
		Attribute attribute_to_modify = attr;
		if (id_ == 0) {
			MPI_Send(&attr, 1, MPI_UINT64_T, recipient_master, 0, MasterComm_);
			MPI_Send(location, 1, attributes_MPI_types_.at(p), recipient_master, 0, MasterComm_);
		} else {
			MPI_Recv(&attribute_to_modify, 1, attributes_MPI_types_.at(p), 0, 0,
				MasterComm_, MPI_STATUS_IGNORE);
			MPI_Recv(agents_.at(recipient_global_id)->GetPointerToAttribute(attribute_to_modify),
				1, attributes_MPI_types_.at(p), 0, 0, MasterComm_, MPI_STATUS_IGNORE);
		}
	} else {
		// Otherwise it is just a memcpy
		memcpy(agents_.at(recipient_global_id)->GetPointerToAttribute(attr),
			location, attributes_sizes_.at(p));
	}
}


ubjson::Value Master::ExportSimulation() {
	// This method is a control method, so sends orders from master 0 to other
	// masters
	if (id_ == 0) {
		order_ = Order::EXPORT_SIMULATION;
		MPI_Bcast(&order_, 1, MPI_INT, 0, MasterComm_);
	}

	ubjson::Value local_agents;
	std::vector<ubjson::Value> local_agents_by_types(nb_types_);
	for (AgentHandler &agent_handler : agent_handlers_) {
		agent_handler.GetJsonNodes(local_agents_by_types);
	}
	for (auto &type : agent_type_to_string_) {
		local_agents[type.second] = std::move(local_agents_by_types.at(type.first));
	}

	// Now all the infos must be gathered in master 0
	std::ostringstream local_data_stream;
	ubjson::StreamWriter<std::ostringstream> writer(local_data_stream);
	writer.writeValue(local_agents);
	std::string local_data = local_data_stream.str();
	int local_data_size = local_data.size();
	// First master 0 must know how much data it will receive
	std::vector<int> sizes_to_receive;
	if (id_ == 0) {
		sizes_to_receive.resize(nb_masters_);
	}
	MPI_Gather(&local_data_size, 1, MPI_INT, sizes_to_receive.data(), 1, MPI_INT, 0, MasterComm_);
	// Storing the results in 'results'
	std::vector<std::string> results;
	if (id_ == 0) {
		for (int i=0; i<nb_masters_; i++) {
			results.emplace_back(std::string(sizes_to_receive.at(i), '0'));
		}
	}
	std::vector<int> displs;
	if (id_ == 0) {
		for (int i=0; i<nb_masters_; i++) {
			displs.push_back(results.at(i).data()-(char*)results.data());
		}
	}
	MPI_Gatherv((void*)local_data.data(), local_data_size, MPI_UNSIGNED_CHAR,
		(void*)results.data(), sizes_to_receive.data(), displs.data(), MPI_UNSIGNED_CHAR, 0, MasterComm_);

	// Grouping the results
	ubjson::Value agents;
	for (auto &master_agents : results) {
		ubjson::Value masters_value;
		std::istringstream s(master_agents);
		ubjson::StreamReader<std::istringstream> reader(s);
		masters_value = reader.getNextValue();
		for (auto &type : agent_type_to_string_) {
			for (auto &agent : masters_value[type.second]) {
				agents[type.second].push_back(agent);
			}
		}
	}
	ubjson::Value final;
	final["agents"] = agents;
	return final;
}


void Master::KillSimulation() {
	if (id_ == 0) {
		order_ = Order::KILL_SIMULATION;
		MPI_Bcast(&order_, 1, MPI_INT, 0, MasterComm_);
	}
}


void Master::WaitOrderFromRoot() {
	if (id_ == 0)
		return;
	while (order_ != Order::KILL_SIMULATION) {
		order_ = Order::IDLE;
		// Waits for an order of master 0
		MPI_Bcast(&order_, 1, MPI_INT, 0, MasterComm_);
		switch (order_) {
			case Order::RUN_SIMULATION: {
				RunSimulation();
				break;
			}
			case Order::CHANGE_PERIOD: {
				ChangePeriod(0);
				break;
			}
			case Order::ADD_AGENTS: {
				// Meaningless vector used to be able to call the following method
				std::vector<void*> artefact = {};
				AddUserAgents(artefact);
				break;
			}
			case Order::MODIFY_ATTRIBUTE: {
				ModifyAttribute();
				break;
			}
			case Order::EXPORT_SIMULATION: {
				ExportSimulation();
				break;
			}
			default:
				continue;
		}
	}
}


void Master::ConvertOutputToInput(std::string in, std::string out) {
	std::ifstream file_in(in, std::ios::in);
	ubjson::StreamReader<std::ifstream> reader(file_in);
	ubjson::Value agents = reader.getNextValue();
	file_in.close();

	ubjson::Value agent_types;
	for (auto &x : agent_type_to_string_) {
		ubjson::Value agent_type;
		agent_type["type"] = x.second;
		agent_type["number"] = (int)agents["agents"][x.second].size();
		agent_type["agents"] = agents["agents"][x.second];
		agent_types.push_back(std::move(agent_type));
	}

	ubjson::Value result;
	result["agent_types"] = agent_types;
	std::ofstream file_out(out, std::ios::out);
	file_out << ubjson::to_ostream(result, ubjson::to_ostream::pretty) << std::endl;
	file_in.close();

}


AgentGlobalId Master::LocalToGlobalId(AgentId id, AgentType type) {
	return nb_types_*id + type;
}


AgentId Master::GlobalToLocalId(AgentGlobalId id) {
	return id / nb_types_;
}


AgentType Master::GlobalToLocalType(AgentGlobalId id) {
	return id % nb_types_;
}


size_t Master::PublicWindowsSize() {
	size_t result;
	MPI_Win_get_attr(public_window_, MPI_WIN_SIZE, &result, NULL);
	return result;
}


size_t Master::CriticalWindowsSize() {
	size_t result;
	MPI_Win_get_attr(critical_window_, MPI_WIN_SIZE, &result, NULL);
	return result;
}


bool Master::IsCritical(Attribute attr, AgentType type) {
	return critical_attributes_.find(std::make_pair(type, attr)) != critical_attributes_.end();
}


bool Master::IsAgentSendable(AgentType type) {
	return non_sendable_agent_types_.find(type) == non_sendable_agent_types_.end();
}


bool Master::IsAttributeSendable(Attribute attr, AgentType type) {
	return attributes_sizes_.find(std::make_pair(attr, type)) != attributes_sizes_.end();
}


bool Master::HasReceivedAttribute(Attribute attr, AgentGlobalId id, void* &location) {
	auto p = received_public_attributes_.get_if_exists(std::make_pair(id, attr));
	if (p.second) {
		location = p.first;
	}
	return p.second;
}


void Master::InitializeAgents(std::vector<void*> &initial_agents) {

	// Initalizes send parameters for master 0
	uint64_t nb_agents = initial_agents.size();

	// Number of agents to send.
	size_t nb_sends;
	if (id_ == 0) {
		nb_sends = nb_agents;
	} else {
		nb_sends = 0;
	}
	std::vector<MPI_Request> send_requests(nb_sends);

	// Informs all masters of the total number of initial agents
	MPI_Bcast(&nb_agents, 1, MPI_UINT64_T, 0, MasterComm_);

	// Stores the masters to which all agents will be sent (built by master 0,
	// sent to the other masters) (in the order of initial_agents)
	std::vector<MasterId> assignment(nb_agents);

	// Stores the agent global ids of initial_agents
	std::vector<AgentGlobalId> agent_ids(nb_agents);
	for (size_t k=0; k<nb_sends; k++) {
		AgentId agent_id = ((AgentStruct*)initial_agents.at(k))->id;
		AgentType agent_type = ((AgentStruct*)initial_agents.at(k))->type;
		agent_ids.at(k) = LocalToGlobalId(agent_id, agent_type);
	}

	// Master 0 assigns and sends the agents, and sends to each master how many
	// agents it will receive and from which type and other infos about the
	// agents
	if (id_ == 0) {
		AssignInitialMasters(initial_agents, assignment, nb_masters_);
	}
	// Sending assignment and agent_ids
	MPI_Bcast(assignment.data(), nb_agents, MPI_INT, 0, MasterComm_);
	MPI_Bcast(agent_ids.data(), nb_agents, MPI_UINT64_T, 0, MasterComm_);

	if (id_ == 0) {
		// Sending agents
		for (size_t k=0; k<nb_sends; k++) {
			AgentStruct *agent = (AgentStruct*)initial_agents.at(k);
			MPI_Isend(agent, 1, agents_MPI_types_.at(agent->type), assignment.at(k),
				0, MasterComm_, &send_requests.at(k));
		}
	}

	// Computing the number of agents to receive and updating the masters info
	size_t nb_receives = 0;
	for (size_t k=0; k<nb_agents; k++) {
		masters_.insert(std::make_pair(agent_ids.at(k), assignment.at(k)));
		if (assignment.at(k) == id_) {
			nb_receives++;
		}
	}

	// Receiving agents, storing them in the same buffer received_agents
	std::vector<MPI_Request> receive_requests(nb_receives);
	utils::fixed_size_multibuffer<AgentStruct> received_agents(max_agent_size_, nb_receives);
	size_t count = 0;
	for (size_t k=0; k<nb_agents; k++) {
		if (assignment.at(k) == id_) {
			AgentType type = GlobalToLocalType(agent_ids.at(k));
			MPI_Irecv(received_agents.pointer_to(count), 1, agents_MPI_types_.at(type),
				0, 0, MasterComm_, &receive_requests.at(count));
			count++;
		}
	}
	MPI_Waitall(nb_receives, receive_requests.data(), MPI_STATUSES_IGNORE);
	MPI_Waitall(nb_sends, send_requests.data(), MPI_STATUSES_IGNORE);

	// Adding agents in the agent handlers
	std::vector<size_t> assignment_agent_handlers(nb_receives);
	AssignInitialAgentHandlers(received_agents, assignment_agent_handlers, agent_handlers_.size());
	for (size_t k=0; k<nb_receives; k++) {
		AddAgent(agent_handlers_.at(assignment_agent_handlers.at(k)), received_agents.pointer_to(k));
	}

	// Now we can initialize the windows
	InitializeWindows(agent_ids);

}


void Master::InitializeWindows(std::vector<AgentGlobalId> &agent_ids) {

	// Sorting the agent global ids so that the next operations will be the same
	// on all masters
	std::sort(agent_ids.begin(), agent_ids.end());

	PublicWindowsDescription.resize(nb_masters_);

	for(auto x: PublicWindowsDescription) {
		x.size = 0;
		x.used = 0;
	}
	CriticalWindowDescription.size = 0;

	// Construction of public_storage_sizes_, critical_storage_size_ and filling
	// of public_agents_offsets_ and critical_agents_offsets_
	// Initialization of maximal_ids_
	maximal_ids_.assign(nb_types_, 0);
	agent_ids_by_types_.resize(nb_types_);
	for (auto &global_id : agent_ids) {
		AgentType type = GlobalToLocalType(global_id);
		AgentType id = GlobalToLocalId(global_id);
		agent_ids_by_types_.at(type).insert(id);
		maximal_ids_.at(type) = std::max(maximal_ids_.at(type), GlobalToLocalId(global_id)+1);
		MasterId master_id = masters_.at(global_id);
		public_agents_offsets_.insert(std::make_pair(global_id, PublicWindowsDescription.at(master_id).used));
		critical_agents_offsets_.insert(std::make_pair(global_id, CriticalWindowDescription.size));
		PublicWindowsDescription.at(master_id).used += public_attributes_struct_sizes_.at(type);
		CriticalWindowDescription.size += critical_attributes_struct_sizes_.at(type);
	}

	// Choosing the size of all public windows
	size_t max_public_used = 0;
	for (auto &x: PublicWindowsDescription) {
		max_public_used = std::max(max_public_used, x.used);
	}

	for (auto &x: PublicWindowsDescription) {
		  x.size = 2*max_public_used;
	}


	// Construction of the windows
	MPI_Win_allocate(2*max_public_used, 1, MPI_INFO_NULL, MasterComm_, &begin_public_window_, &public_window_);
	MPI_Win_allocate(2*CriticalWindowDescription.size, 1, MPI_INFO_NULL, MasterComm_, &begin_critical_window_, &critical_window_);

	// Now that the agents were received, fills the windows with their content
	FillWindows(agent_ids);

}


void Master::FillWindows(std::vector<AgentGlobalId> &agent_ids) {

	for (auto &global_id : agent_ids) {
		// Copying
		if (masters_.at(global_id) == id_) {
			Agent* agent = agents_.at(global_id);
			void* public_location = static_cast<char*>(begin_public_window_) + public_agents_offsets_.at(global_id);
			void* critical_location = static_cast<char*>(begin_critical_window_) + critical_agents_offsets_.at(global_id);
			agent->CopyPublicAttributes(public_location);
			agent->CopyCriticalAttributes(critical_location);
		}
		if (critical_structs_MPI_types_.find(GlobalToLocalType(global_id))
			  != critical_structs_MPI_types_.end())
		{
			void* location = static_cast<char*>(begin_critical_window_) + critical_agents_offsets_.at(global_id);
			MPI_Bcast(location, 1, critical_structs_MPI_types_.at(GlobalToLocalType(global_id)),
				masters_.at(global_id), MasterComm_);
		}
	}

}


void Master::AddAgent(AgentHandler &agent_handler, void* structure) {
	std::unique_ptr<Agent> new_agent = Agent::FromStruct(structure, id_, *this);
	AgentGlobalId new_agent_id_ = LocalToGlobalId(new_agent->id_, new_agent->type_);

	// Warning: do not use new_agent after this operation
	agents_.insert(std::make_pair(new_agent_id_, agent_handler.AddAgent(std::move(new_agent))));
}


void Master::Synchronize() {
	// Synchronizes the masters using MPI
	MPI_Barrier(MasterComm_);
}


void Master::DistributeReceivedInteractions() {
	AgentGlobalId agent;
	for (auto &inter : received_interactions_) {
		agent = LocalToGlobalId(inter->recipient_id_, inter->recipient_type_);
		agents_.at(agent)->ReceiveMessage(inter);
	}
	received_interactions_.clear();
}


void Master::RunBehaviors() {
	received_public_attributes_.clear();
	stored_public_attributes_.clear();
	size_t n = agent_handlers_.size();
	MPI_Win_lock_all(MPI_MODE_NOCHECK, public_window_);
	std::vector<std::thread> threads;
	for (size_t i=0; i<n; i++) {
		threads.emplace_back(&AgentHandler::RunBehaviors, &(agent_handlers_.at(i)));
	}
	for (size_t i=0; i<n; i++) {
		threads.at(i).join();
	}
	MPI_Win_unlock_all(public_window_);
}


void* Master::GetPublicAttribute(Attribute attr, AgentGlobalId recipient) {
	AgentType agent_type = GlobalToLocalType(recipient);
	auto p_type  = std::make_pair(agent_type, attr);
	auto p_id = std::make_pair(recipient, attr);
	void* location = nullptr;
	if (HasReceivedAttribute(attr, recipient, location)) {
		return location;
	} else {
		MasterId master_recipient_id = masters_.at(recipient);
		MPI_Datatype MPI_attr_type = attributes_MPI_types_.at(p_type);
		size_t target_disp = PublicTargetDisp(recipient, attr);
		void* storage_location = stored_public_attributes_.allocate(attributes_sizes_.at(p_type));
		received_public_attributes_.set(p_id, storage_location);
		MPI_Get(storage_location, 1, MPI_attr_type, master_recipient_id,
			target_disp, 1, MPI_attr_type, public_window_);
		return storage_location;
	}
}


void* Master::GetCriticalAttribute(Attribute attr, AgentGlobalId recipient) {
	auto p = std::make_pair(GlobalToLocalType(recipient), attr);
	size_t target_disp = critical_agents_offsets_.at(recipient) + critical_attributes_offsets_.at(p);
	void* target_location = static_cast<char*>(begin_critical_window_) + target_disp;
	return target_location;
}


void Master::UpdateAllPublicAttributes() {
	size_t n = agent_handlers_.size();
	std::vector<std::thread> threads;
	MPI_Win_lock_all(MPI_MODE_NOCHECK, critical_window_);
	for (size_t i=0; i<n; i++) {
		threads.emplace_back(&AgentHandler::UpdateAllPublicAttributes, &(agent_handlers_.at(i)));
	}
	for (size_t i=0; i<n; i++) {
		threads.at(i).join();
	}
	MPI_Win_unlock_all(critical_window_);
}


void Master::SendReceiveInteractions() {
	/* First each master receives how many interactions from each type it will
	 * receive from each master                                               */
	int total_to_send = 0;
	int total_to_receive = 0;
	std::vector<int> nb_messages_to_send(nb_masters_*nb_interactions_);
	std::vector<int> nb_messages_to_receive(nb_masters_*nb_interactions_);
	for (int i=0; i<nb_masters_*nb_interactions_; i++) {
		nb_messages_to_send.at(i) = interactions_to_send_.at(i).size();
		total_to_send += nb_messages_to_send.at(i);
	}
	MPI_Alltoall(nb_messages_to_send.data(), nb_interactions_, MPI_INT,
		nb_messages_to_receive.data(), nb_interactions_, MPI_INT, MasterComm_);
	for (int i=0; i<nb_masters_*nb_interactions_; i++) {
		total_to_receive += nb_messages_to_receive.at(i);
	}

	std::vector<MPI_Request> requests(total_to_receive+total_to_send);

	// TODO: Optimize this.
	// Possible optimization: sending and receiving all messages of the same
	// type at the same time. It requires several containers for each type of
	// message, for both sending and receiving. It is more complicated but may
	// be far better in terms of execution time.

	// Message sending
	int count = 0;
	for (int i=0; i<nb_masters_; i++) {
		for (int j=0; j<nb_interactions_; j++) {
			for (int k=0; k<nb_messages_to_send.at(i*nb_interactions_+j); k++) {
				MPI_Isend(interactions_to_send_.at(i*nb_interactions_+j).raw().at(k).get()->GetStructure(),
					1, interactions_MPI_types_.at(j), i, 0, MasterComm_, requests.data() + count);
				count++;
			}
		}
	}

	// Message receiving
	if (interactions_buffer_.size() < total_to_receive) {
		interactions_buffer_.resize(total_to_receive);
	}
	count = 0;
	for (int i=0; i<nb_masters_; i++) {
		for (int j=0; j<nb_interactions_; j++) {
			for (int k=0; k<nb_messages_to_receive.at(i*nb_interactions_+j); k++) {
				MPI_Irecv(interactions_buffer_.pointer_to(count),
					1, interactions_MPI_types_.at(j), i, 0, MasterComm_, requests.data() + total_to_send + count);
				count++;
			}
		}
	}

	MPI_Waitall(total_to_receive+total_to_send, requests.data(), MPI_STATUSES_IGNORE);

	for (int k=0; k<total_to_receive; k++) {
		received_interactions_.push_back(Interaction::FromStruct(interactions_buffer_.pointer_to(k)));
	}

	// Finally the sent interactions are deleted
	for (auto &x: interactions_to_send_) {
		x.clear();
	}
}


void Master::MetaEvolution() {
	// Will read AgentsToDelete and AgentsToCreate to fill
	// LocalMetaEvolutionDescriptions with the associated evolutions.
	auto lock = AgentsToDelete.unique_lock();
	for(auto AgentId: AgentsToDelete.raw()) {
		MetaEvolutionDescription _desc = MetaEvolutionDescription();
		// Initialisation
		_desc.type = AgentEvolution::Death;
		_desc.agent_id = AgentId;
		_desc.origin_id = id_;
		_desc.destination_id = 0;
		_desc.private_overhead = 0;
		LocalMetaEvolutionDescriptions.push_back(_desc);
	}
	lock.unlock();

	// Will then call heuristics:MigrateAgents to continue filling
	// this very same vector with the migrations needed.
	MigrateAgents();
	// Use a MPI_Allgather to exchange the number of meta evolution on each
	// master

        int *MetaEvolutionsCount = (int*) calloc(nb_masters_, sizeof(int));
        int LocalCount = LocalMetaEvolutionDescriptions.size();
        MPI_Allgather(&LocalCount, 1, MPI_INT, MetaEvolutionsCount, 1, MPI_INT, MasterComm_);
        // Now, MetaEvolutionsCount is an array containing, for each master, the number of MetaEvolution it has to send
	// After that, use MPI_Allgatherv to exchange (number obtained at the previous step)
	// the meta evolutions with all the masters and put them in a Global Vector

        int sum=0;
        for(int i=0; i< nb_masters_; ++i) {
                sum+=MetaEvolutionsCount[i];
        }

		// TODO: Verify the following (before: sizeof(int), sum)
        int *disps = (int *) calloc(sizeof(int), nb_masters_);
        disps[0]=0;
        for(int i=1; i< nb_masters_; ++i) {
                disps[i]=disps[i-1]+MetaEvolutionsCount[i];
        }

	GlobalMetaEvolutionDescriptions.resize(sum);
	// FIXME: try the evolution in place in the local vector
        MPI_Allgatherv(LocalMetaEvolutionDescriptions.data(), LocalMetaEvolutionDescriptions.size(), MetaEvolutionDescriptionMPIDatatype, (GlobalMetaEvolutionDescriptions.data()), MetaEvolutionsCount, disps, MetaEvolutionDescriptionMPIDatatype, MasterComm_);
	// Then use all the meta evolutions to actually migrate agents, pop them on
	// our local master or on another master, and destruct the agents that died.
}


void Master::RunTimeStep() {
	step_++;
	// TODO: updating environments
	UpdateAllPublicAttributes();
	Synchronize();
	//MetaEvolution();
	Synchronize();
	SendReceiveInteractions();
	Synchronize();
	DistributeReceivedInteractions();
	Synchronize();
	RunBehaviors();
	Synchronize();
}
