/**
 * \file types.hpp
 * \brief Names the types used in all classes of the simulation core.
 */

#ifndef TYPES_HPP_
#define TYPES_HPP_

#include <vector>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <mpi.h>

#include "utils.hpp"


class Agent;
class AgentHandler;
class Master;
class Interaction;

/**
 * \struct hash_pair
 * \brief Simple hash function over pairs <P, Q> where P and Q are hashable.
 */
template <typename P, typename Q>
struct hash_pair {
	size_t operator()(std::pair<P, Q> p) const {
		return std::hash<P>()(p.first) ^ std::hash<Q>()(p.second);
	}
};

// Ids of agents / masters / interactions
typedef uint64_t Tag;
typedef uint64_t AgentId;
typedef uint64_t AgentGlobalId;
typedef int MasterId;

// Ids of types
typedef uint64_t AgentType;
typedef uint64_t InteractionType;
typedef uint64_t MessageType;

// Id of agent attributes
typedef uint64_t Attribute;

// Containers, iterators
typedef std::vector<std::unique_ptr<Interaction>> InteractionContainer;
typedef utils::thread_safe_vector<std::unique_ptr<Interaction>> InteractionContainerThreadSafe;
typedef std::vector<InteractionContainerThreadSafe> InteractionMatrix;
typedef std::unordered_map<std::pair<AgentId, AgentType>, std::unique_ptr<Agent>, hash_pair<AgentId, AgentType>> AgentContainer;

// Names
typedef std::string AgentName;
typedef std::string AttributeName;

// Time step
typedef uint64_t Time;

typedef utils::thread_safe_unordered_map<std::pair<AgentGlobalId, Attribute>, void*, hash_pair<AgentGlobalId, Attribute>> ReceivedAttributesThreadSafe;

// Maps and sets with pairs or vectors
typedef std::unordered_set<std::pair<AgentType, Attribute>, hash_pair<AgentType, Attribute>> CriticalAttributes;
typedef std::unordered_map<std::pair<AgentType, Attribute>, size_t, hash_pair<AgentType, Attribute>> AttributesSizes;
typedef std::unordered_map<std::pair<AgentType, Attribute>, MPI_Datatype, hash_pair<AgentType, Attribute>> AttributesMPITypes;
typedef std::unordered_map<std::pair<AgentType, Attribute>, size_t, hash_pair<AgentType, Attribute>> AttributesOffsets;
typedef std::unordered_map<std::pair<AgentType, Attribute>, AttributeName, hash_pair<AgentType, Attribute>> AttributesNames;
typedef std::unordered_map<std::pair<AgentName, AttributeName>, std::pair<AgentType, Attribute>, hash_pair<AgentName, AttributeName>> AttributesIds;


/**
 * \class AgentNotFound
 * \brief Exception launched when a public attribute of a non-existing agent is
 *        asked.
 */
class AgentNotFound : public std::runtime_error {
public:
	AgentNotFound(AgentId id, std::string type) :
		std::runtime_error{
			"Trying to access an attribute of agent " + std::to_string(id) +
				" of type " + type + ", which does not exist."
		}
	{
	}
};


// Meta-Evolution type
enum class AgentEvolution { Birth, Death, Migration };

// Description of a Meta-Evolution of an agent
typedef struct _MetaEvolutionDescription {
	// Type of the Meta-Evolution
	// If the type is Death (resp. Birth), then the destination_id (resp. origin_id) is ignored
	AgentEvolution type;
	AgentGlobalId agent_id;
	MasterId origin_id;
	MasterId destination_id;
	/* private_overhead represents the overehead in bytes needed to represent the private structure of an agent.
	currently, since we do not migrate the data from private structures in agent, it's likely that it is equal
	to zero */
	size_t private_overhead;
} MetaEvolutionDescription;

void generateMPIDatatype(MPI_Datatype &type);

// Description of a window (public or private)
typedef struct _WindowDescription {
      size_t size;
      size_t used;
} WindowDescription;
#endif
