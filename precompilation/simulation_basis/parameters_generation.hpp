/**
 * \file parameters_generation.hpp
 * \brief Defines the functions that will load values, types and sizes of
 *        attributes defined in the model (generated in the precompilation step).
 */

#ifndef PARAMETERS_GENERATION_HPP_
#define PARAMETERS_GENERATION_HPP_

#include "types.hpp"


/**
 * \fn void CreateAttributesMPIDatatypes(AttributesMPITypes &attributes_MPI_types)
 * \brief Fills the attributes_MPI_types_ of a master.
 * \param attributes_MPI_types Reference to an attributes_MPI_types_ of a master.
 * \remark Generated in the precompilation step.
 * \see Master
 */
void CreateAttributesMPIDatatypes(AttributesMPITypes &attributes_MPI_types);

/**
 * \fn size_t CreateAgentsMPIDatatypes(
 *                std::unordered_map<AgentType, MPI_Datatype> &agents_MPI_types,
 *                AttributesMPITypes &attributes_MPI_types)
 * \brief Fills the agents_MPI_types_ of a master..
 * \param agents_MPI_types Reference to an agents_MPI_types_ of a master.
 * \param attributes_MPI_types Reference to an attributes_MPI_types_ of a master.
 * \warning CreateAttributesMPIDatatypes must be called on attributes_MPI_types
 *          before this function, in order for it to work correctly.
 * \remark Generated in the precompilation step.
 * \see Master
 */
size_t CreateAgentsMPIDatatypes(
	std::unordered_map<AgentType, MPI_Datatype> &agents_MPI_types,
	AttributesMPITypes &attributes_MPI_types);

/**
 * \fn void CreateCriticalStructsMPIDatatypes(
 *             std::unordered_map<AgentType, MPI_Datatype> &critical_structs_MPI_types,
 *             AttributesMPITypes &attributes_MPI_types)
 * \brief Fills the critical_structs_MPI_types_ of a master.
 * \param critical_structs_MPI_types_ Reference to an critical_structs_MPI_types_
 *        of a master.
 * \param attributes_MPI_types Reference to an attributes_MPI_types_ of a master.
 * \warning CreateAttributesMPIDatatypes must be called on attributes_MPI_types
 *          before this function, in order for it to work correctly.
 * \remark Generated in the precompilation step.
 * \see Master
 */
void CreateCriticalStructsMPIDatatypes(
	std::unordered_map<AgentType, MPI_Datatype> &critical_structs_MPI_types,
	AttributesMPITypes &attributes_MPI_types);

/**
 * \fn size_t CreateInteractionsMPIDatatypes(
 *        std::unordered_map<InteractionType, MPI_Datatype> &interactions_MPI_types)
 * \brief Fills the interactions_MPI_types_ of a master.
 * \param interactions_MPI_types Reference to an interactions_MPI_types_ of a
 *        master.
 * \remark Generated in the precompilation step.
 * \see Master
 */
size_t CreateInteractionsMPIDatatypes(
	std::unordered_map<InteractionType, MPI_Datatype> &interactions_MPI_types);

/**
 * \fn void CreateAttributesSizes(AttributesSizes &attributes_sizes)
 * \brief Fills the attributes_sizes_ of a master.
 * \param attributes_sizes Reference to an attributes_sizes_ of a master.
 * \remark Generated in the precompilation step.
 * \see Master
 */
void CreateAttributesSizes(AttributesSizes &attributes_sizes);

/**
 * \fn void CreateCriticalAttributes(CriticalAttributes &critical_attributes)
 * \brief Fills the critical_attributes_ of a master.
 * \param critical_attributes Reference to an critical_attributes_ of a master.
 * \remark Generated in the precompilation step.
 * \see Master
 */
void CreateCriticalAttributes(CriticalAttributes &critical_attributes);

/**
 * \fn void CreateNonSendableAgentTypes(std::unordered_set<AgentType> &non_sendable_agent_types)
 * \brief Fills the non_sendable_agent_types_ of a master.
 * \param non_sendable_agent_types Reference to a non_sendable_agent_types_ of a
 *        master.
 * \remark Generated in the precompilation step.
 * \see Master
 */
void CreateNonSendableAgentTypes(std::unordered_set<AgentType> &non_sendable_agent_types);

/**
 * \fn void CreatePublicAttributesOffsets(AttributesOffsets &public_attributes_offsets)
 * \brief Fills the public_attributes_offsets_ of a master.
 * \param public_attributes_offsets Reference to a public_attributes_offsets_
 *        of a master.
 * \remark Generated in the precompilation step.
 * \see Master
 */
void CreatePublicAttributesOffsets(AttributesOffsets &public_attributes_offsets);

/**
 * \fn void CreatePublicStructSizes(std::unordered_map<AgentType, size_t> public_attributes_struct_sizes)
 * \brief Fills the public_attributes_struct_sizes_ of a master.
 * \param public_attributes_struct_sizes Reference to a public_attributes_struct_sizes_
 *        of a master.
 * \remark Generated in the precompilation step.
 * \see Master
 */
void CreatePublicStructSizes(std::unordered_map<AgentType, size_t> &public_attributes_struct_sizes);

/**
 * \fn void CreateCriticalAttributesOffsets(AttributesOffsets &critical_attributes_offsets)
 * \brief Fills the critical_attributes_offsets_ of a master.
 * \param critical_attributes_offsets Reference to a critical_attributes_offsets_
 *        of a master.
 * \remark Generated in the precompilation step.
 * \see Master
 */
void CreateCriticalAttributesOffsets(AttributesOffsets &critical_attributes_offsets);

/**
 * \fn void CreateCriticalStructSizes(std::unordered_map<AgentType, size_t> &critical_attributes_struct_sizes)
 * \brief Fills the critical_attributes_struct_sizes_ of a master.
 * \param critical_attributes_struct_sizes Reference to a critical_attributes_struct_sizes_
 *        of a master.
 * \remark Generated in the precompilation step.
 * \see Master
 */
void CreateCriticalStructSizes(std::unordered_map<AgentType, size_t> &critical_attributes_struct_sizes);

/**
 * \fn void CreateAgentsNamesRelation(
 *               std::unordered_map<AgentType, AgentName> &agent_type_to_string,
 *               std::unordered_map<AgentName, AgentType> &string_to_agent_type)
 * \brief Fills the agent_type_to_string_ and string_to_agent_type_ of a master.
 * \param agent_type_to_string Reference to a agent_type_to_string_ of a master.
 * \param string_to_agent_type Reference to a string_to_agent_type_ of a master.
 * \remark Generated in the precompilation step.
 * \see Master
 */
void CreateAgentsNamesRelation(
	std::unordered_map<AgentType, AgentName> &agent_type_to_string,
	std::unordered_map<AgentName, AgentType> &string_to_agent_type);

/**
 * \fn void CreateAttributesNamesRelation(
 *             AttributesNames &attribute_to_string, AttributesIds &string_to_attribute)
 * \brief Fills the attribute_to_string_ and string_to_attribute_ of a master.
 * \param attribute_to_string Reference to a attribute_to_string_ of a master.
 * \param string_to_attribute Reference to a string_to_attribute_ of a master.
 * \remark Generated in the precompilation step.
 * \see Master
 */
void CreateAttributesNamesRelation(
	AttributesNames &attribute_to_string, AttributesIds &string_to_attribute);

/**
 * \fn AgentType NbAgentTypes()
 * \brief Returns the number of agent types in the model.
 * \remark Generated in the precompilation step.
 */
AgentType NbAgentTypes();

/**
 * \fn InteractionType NbInteractionTypes()
 * \brief Returns the number of interaction types in the model.
 * \remark Generated in the precompilation step.
 */
InteractionType NbInteractionTypes();


// TODO: Uncomment once precompilation handled constant
// /**
//  * \fn void GenerateConstants(std::unordered_map<std::string, void*> &constants_)
//  * \brief Creates and allocates the constants of the simulation and fills
//  *        the attribute constants_ of a master.
//  * \param constants_ Reference to a constants_ of a master.
//  */
// void GenerateConstants(std::unordered_map<std::string, void*> &constants_);


#endif
