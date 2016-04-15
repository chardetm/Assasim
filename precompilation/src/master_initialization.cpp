/**
 * \file master_initialization.cpp
 */

#include "master_initialization.hpp"
#include "mpi_func.hpp"
#include <sstream>


std::string GenerateAttributesStruct(Model &model) {
	std::stringstream stream;

	for (const auto &agent : model.GetAgents()) {
		stream << agent.second.AttributesStruct(agent.first);
		stream << agent.second.PublicAttributesStruct(agent.first);
		stream << agent.second.CriticalAttributesStruct(agent.first);
		stream << agent.second.MessageStruct(agent.first);
	}

	for (const auto &interaction : model.GetInteractions()) {
		stream << interaction.second.AttributesStruct(interaction.first);
		stream << interaction.second.MessageStruct(interaction.first);
	}

	return stream.str();
}


std::string GenerateAttributesMPIDatatypesFunction(Model &model, clang::ASTContext *context) {
	std::stringstream stream;

	// Add prototype
	stream << "void CreateAttributesMPIDatatypes(AttributesMPITypes &attributes_MPI_types) {\n"
		   << "\tstd::vector<int> lengths; "
		   << "std::vector<MPI_Aint> offsets; "
		   << "std::vector<MPI_Datatype> mpi_types;\n"
		   << "\tMPI_Datatype t;\n";
	// Generates the MPI_Datatype of each attribute and add it in the map
	std::unordered_set<std::string> temp_database;
	temp_database.insert("t");
	for (const auto &agent : model.GetAgents()) {
		for (const auto& field : agent.second.GetFields()) {
			if (!field.second.IsSendable())
				continue; // Ignore not sendable fields
			std::string code = GenerateCodeMPIDatatype(field.second.GetType(), context, "t", temp_database);
			if (code.substr(0,6) == "MPI_Da" || code.substr(0,3) != "MPI") // If it is a structure, add the code
				stream << code
					   << "\tattributes_MPI_types[std::pair<AgentType, Attribute>("
					   << agent.second.GetId() << ", " << field.second.GetId() << ")]"
					   << " = t;\n";
			else // it is just a constant
				stream << "\tattributes_MPI_types[std::pair<AgentType, Attribute>("
					   << agent.second.GetId() << ", " << field.second.GetId() << ")]"
					   << " = " << code << ";\n";
		}
	}
	stream << "}\n";
	return stream.str();
}


std::string GenerateAgentsMPIDatatypesFunction(Model &model) {
	std::stringstream stream;

	// Add prototype
	stream << "size_t CreateAgentsMPIDatatypes(std::unordered_map<AgentType, MPI_Datatype> &agents_MPI_types, AttributesMPITypes &attributes_MPI_types) {\n"
		   << "\tstd::vector<int> lengths; "
		   << "std::vector<MPI_Aint> offsets; "
		   << "std::vector<MPI_Datatype> mpi_types;\n"
		   << "\tMPI_Datatype t;\n"
		   << "\tsize_t max_size = 0;\n";

	// Generate the MPI_Datatype for the struct containing the data
	for (const auto &agent : model.GetAgents()) {
		// First the lengths (all 1)
		stream << "\tlengths = {";
		int n_fields = 0;
		for (const auto &field : agent.second.GetFields()) {
			if (field.second.IsSendable())
				n_fields++;
		}
		for (int i = 0; i < n_fields; i++) {
			stream << "1,";
		}
		stream.seekp(-1, std::ios_base::cur);
		stream << "};\n";
		// Then the offsets
		stream << "\toffsets = {";
		for (const auto &field : agent.second.GetFields()) {
			if (field.second.IsSendable())
				stream << "offsetof(" << agent.first << "Attrs"
					   << "," << field.first
					   << "),";
		}
		if (n_fields > 0)
			stream.seekp(-1, std::ios_base::cur);
		stream << "};\n";
		// The MPI_Datatypes
		stream << "\tmpi_types = {";
		for (const auto &field : agent.second.GetFields()) {
			if (field.second.IsSendable())
				stream << "attributes_MPI_types[std::pair<AgentType, Attribute>("
					   << agent.second.GetId() << "," << field.second.GetId()
					   << ")],";
		}
		if (n_fields > 0)
			stream.seekp(-1, std::ios_base::cur);
		stream << "};\n";
		stream << "\tMPI_Type_create_struct(" << n_fields
			   << ", lengths.data(), offsets.data(), mpi_types.data(), &t);\n"
			   << "\tMPI_Type_commit(&t);\n";
		// Now the MPI_Datatype of the message structure
		// i.e. we add the type and the id and types of the sender and recipient
		stream << "\tlengths = {1,1,1};\n"
			   << "\toffsets = {offsetof(" << agent.first << "MessageStruct,id),"
			   << "offsetof(" << agent.first << "MessageStruct,type),"
			   << "offsetof(" << agent.first << "MessageStruct,data)"
			   << "};\n";
		stream << "\tmpi_types = {MPI_UINT64_T,MPI_UINT64_T,t};\n";
		stream << "\tMPI_Type_create_struct(" << "3"
			   << ", lengths.data(), offsets.data(), mpi_types.data(), &t);\n"
			   << "\tMPI_Type_commit(&t);\n";

		// Store the MPI_Datatype
		stream << "\tagents_MPI_types[" << agent.second.GetId()
			   << "] = t;\n";
		// Update the maximum size
		stream << "\tif (sizeof(" << agent.first << "MessageStruct) > max_size)"
			   << " {max_size = sizeof(" << agent.first << "MessageStruct);}\n";
	}
	stream << "\treturn max_size;\n"
		   << "}\n";

	return stream.str();
}


std::string GenerateCriticalStructsMPIDatatypesFunction(Model &model) {
	std::stringstream stream;

	// Add prototype
	stream << "void CreateCriticalStructsMPIDatatypes(std::unordered_map<AgentType, MPI_Datatype> &critical_structs_MPI_types, AttributesMPITypes &attributes_MPI_types) {\n"
		   << "\tstd::vector<int> lengths; "
		   << "std::vector<MPI_Aint> offsets; "
		   << "std::vector<MPI_Datatype> mpi_types;\n"
		   << "\tMPI_Datatype t;\n";

	// Generate the MPI_Datatype for the struct containing the data
	for (const auto &agent : model.GetAgents()) {
		if (!agent.second.IsSendable())
			continue;
		// First the lengths (all 1)
		int n_fields = 0;
		for (const auto &field : agent.second.GetFields()) {
			if (field.second.IsCritical()) {
				n_fields++;
			}
		}
		if (n_fields == 0)
			continue;
		stream << "\tlengths = {";
		for (int i = 0; i<n_fields; ++i)
				stream << "1,";
		stream.seekp(-1, std::ios_base::cur);
		stream << "};\n";
		// Then the offsets
		stream << "\toffsets = {";
		for (const auto &field : agent.second.GetFields()) {
			if (field.second.IsCritical())
				stream << "offsetof(" << agent.first << "CriticalAttrs"
					   << "," << field.first
					   << "),";
		}
		stream.seekp(-1, std::ios_base::cur);
		stream << "};\n";
		// The MPI_Datatypes
		stream << "\tmpi_types = {";
		for (const auto &field : agent.second.GetFields()) {
			if (field.second.IsCritical())
				stream << "attributes_MPI_types[std::pair<AgentType, Attribute>("
					   << agent.second.GetId() << "," << field.second.GetId()
					   << ")],";
		}
		stream.seekp(-1, std::ios_base::cur);
		stream << "};\n";
		stream << "\tMPI_Type_create_struct(" << n_fields
			   << ", lengths.data(), offsets.data(), mpi_types.data(), &t);\n"
			   << "\tMPI_Type_commit(&t);\n";
		// Store the MPI_Datatype
		stream << "\tcritical_structs_MPI_types[" << agent.second.GetId()
			   << "] = t;\n";
	}
	stream << "}\n";

	return stream.str();
}


std::string GenerateInteractionsMPIDatatypesFunction(Model &model, clang::ASTContext *context) {
	std::stringstream stream;

	// Add prototype
	stream << "size_t CreateInteractionsMPIDatatypes(std::unordered_map<InteractionType, MPI_Datatype> &interactions_MPI_types) {\n"
		   << "\tstd::vector<int> lengths; "
		   << "std::vector<MPI_Aint> offsets; "
		   << "std::vector<MPI_Datatype> mpi_types;\n"
		   << "\tMPI_Datatype t;\n"
		   << "\tsize_t max_size = 0;\n";

	std::unordered_set<std::string> temp_database;
	temp_database.insert("t");

	for (const auto &interaction : model.GetInteractions()) {
		int n_fields = interaction.second.GetFields().size();
		std::map<int64_t, std::string> type_temporaries;
		int i = 0;
		// Construct the data types of the fields
		for (const auto &field : interaction.second.GetFields()) {
			std::string temp = "t" + std::to_string(i);
			if (!temp_database.count(temp)) {
				stream << "\tMPI_Datatype " << temp << ";\n";
				temp_database.insert(temp);
			}

			std::string code_field = GenerateCodeMPIDatatype(field.second.GetType(), context, temp, temp_database);
			if (code_field.substr(0,6) != "MPI_Da" && code_field.substr(0,3) == "MPI") // No temporary to use
				type_temporaries[field.second.GetId()] = code_field;
			else {
				type_temporaries[field.second.GetId()] = temp;
				stream << code_field;
			}
			i++;
		}
		// Now construct the corresponding struct MPI_Datatype
		stream << "\tlengths = {";
		for (int j = 0; j < n_fields; j++)
			stream << "1,";
		if (n_fields > 0)
			stream.seekp(-1,std::ios_base::cur);
		stream << "};\n";

		stream << "\toffsets = {";
		for (const auto &field : interaction.second.GetFields()) {
			stream << "offsetof(" << interaction.first << "Attrs"
				   << "," << field.first
				   << "),";
		}
		if (n_fields > 0)
			stream.seekp(-1,std::ios_base::cur);
		stream << "};\n";

		stream << "\tmpi_types = {";
		for (const auto &field : interaction.second.GetFields()) {
			stream << type_temporaries[field.second.GetId()] <<",";
		}
		if (n_fields > 0)
			stream.seekp(-1,std::ios_base::cur);
		stream << "};\n";

		stream << "\tMPI_Type_create_struct(" << n_fields
			   << ", lengths.data(), offsets.data(), mpi_types.data(), &t);\n"
			   << "\tMPI_Type_commit(&t);\n";
		// Now the MPI_Datatype of the message structure
		// i.e. we add the type and the id and types of the sender and recipient
		stream << "\tlengths = {1,1,1,1,1,1};\n"
			   << "\toffsets = {offsetof(" << interaction.first << "MessageStruct,type),"
			   << "offsetof(" << interaction.first << "MessageStruct,sender_id),\n"
			   << "\t           offsetof(" << interaction.first << "MessageStruct,sender_type),"
			   << "offsetof(" << interaction.first << "MessageStruct,recipient_id),\n"
			   << "\t           offsetof(" << interaction.first << "MessageStruct,recipient_type),"
			   << "offsetof(" << interaction.first << "MessageStruct,data)}"
			   << ";\n";
		stream << "\tmpi_types = {MPI_UINT64_T,MPI_UINT64_T,MPI_UINT64_T,MPI_UINT64_T,MPI_UINT64_T,t};\n";
		stream << "\tMPI_Type_create_struct(" << "6"
			   << ", lengths.data(), offsets.data(), mpi_types.data(), &t);\n"
			   << "\tMPI_Type_commit(&t);\n";
		// Free the intermediary generated MPI_Datatypes
		for (const auto &temporary : type_temporaries) {
			if (temporary.second.length() > 0 && temporary.second.substr(0,1) == "t") // if it represents a constructed MPI_Datatype, free it
				stream << "\tMPI_Type_free(&" << temporary.second <<");\n";
		}
		// Store the MPI_Datatype
		stream << "\tinteractions_MPI_types[" << interaction.second.GetId()
			   << "] = t;\n";
		// Update the maximum size
		stream << "\tif (sizeof(" << interaction.first << "MessageStruct) > max_size)"
			   << " {max_size = sizeof(" << interaction.first << "MessageStruct);}\n";
	}

	stream << "\treturn max_size;\n"
		   << "}\n";

	return stream.str();
}


std::string GenerateAttributesSizeFunction(Model &model) {
	std::stringstream stream;
	// Add prototype
	stream << "void CreateAttributesSizes(AttributesSizes &attributes_sizes) {\n";

	for (const auto &agent : model.GetAgents()) {
		for (const auto &field : agent.second.GetFields()) {
			if (!field.second.IsSendable())
				continue;
			stream << "\tattributes_sizes[std::pair<AgentType, Attribute>("
				   << agent.second.GetId() << "," << field.second.GetId()
				   << ")] = sizeof(" << GetTypeAsString(field.second.GetType())
				   << ");\n";
		}
	}
	stream << "}\n";

	return stream.str();
}


std::string GenerateCriticalAttributesFunction(Model &model) {
	std::stringstream stream;
	// Add prototype
	stream << "void CreateCriticalAttributes(CriticalAttributes &critical_attributes) {\n";

	for (const auto &agent : model.GetAgents()) {
		for (const auto &field : agent.second.GetFields()) {
			if (field.second.IsCritical())
				stream << "\tcritical_attributes.insert(std::pair<AgentType, Attribute>("
					   << agent.second.GetId() << "," << field.second.GetId()
					   << "));\n";
		}
	}
	stream << "}\n";

	return stream.str();
}

std::string GenerateNonSendableAgentTypesFunction(Model &model) {
	std::stringstream stream;
	// Add prototype
	stream << "void CreateNonSendableAgentTypes(std::unordered_set<AgentType> &non_sendable_agents) {\n";
	for (const auto &agent : model.GetAgents()) {
		if (!agent.second.IsSendable())
			stream << "\tnon_sendable_agents.insert(" << agent.second.GetId() << ");\n";
	}
	stream << "}\n";
	return stream.str();
}

std::string GeneratePublicAttributesOffsetsFunction(Model &model) {
	std::stringstream stream;
	// Add prototype
	stream << "void CreatePublicAttributesOffsets(AttributesOffsets &public_attributes_offsets) {\n";

	for (const auto &agent : model.GetAgents())
		for (const auto &field : agent.second.GetFields()) {
			if (field.second.GetAccess() == clang::AS_public && !field.second.IsCritical())
				stream << "\tpublic_attributes_offsets[std::pair<AgentType, Attribute>("
					   << agent.second.GetId() << "," << field.second.GetId()
					   << ")] = "
					   << "offsetof(" << agent.first << "PublicAttrs"
					   << "," << field.first
					   << ");\n";
	}
	stream << "}\n";

	return stream.str();
}


std::string GeneratePublicStructSizesFunction(Model &model) {
	std::stringstream stream;
	// Add prototype
	stream << "void CreatePublicStructSizes(std::unordered_map<AgentType, size_t> &public_attributes_struct_sizes) {\n";

	for (const auto &agent : model.GetAgents()) {
		stream << "\tpublic_attributes_struct_sizes[" << agent.second.GetId()
			   << "] = sizeof(" << agent.first << "PublicAttrs);\n";
	}
	stream << "}\n";

	return stream.str();
}


std::string GenerateCriticalAttributesOffsetsFunction(Model &model) {
	std::stringstream stream;
	// Add prototype
	stream << "void CreateCriticalAttributesOffsets(AttributesOffsets &critical_attributes_offsets) {\n";

	for (const auto &agent : model.GetAgents())
		for (const auto &field : agent.second.GetFields()) {
			if (field.second.IsCritical())
				stream << "\tcritical_attributes_offsets[std::pair<AgentType, Attribute>("
					   << agent.second.GetId() << "," << field.second.GetId()
					   << ")] = "
					   << "offsetof(" << agent.first << "CriticalAttrs"
					   << "," << field.first
					   << ");\n";
	}
	stream << "}\n";

	return stream.str();
}


std::string GenerateCriticalStructSizesFunction(Model &model) {
	std::stringstream stream;
	// Add prototype
	stream << "void CreateCriticalStructSizes(std::unordered_map<AgentType, size_t> &critical_attributes_struct_sizes) {\n";

	for (const auto &agent : model.GetAgents()) {
		stream << "\tcritical_attributes_struct_sizes[" << agent.second.GetId()
			   << "] = sizeof(" << agent.first << "CriticalAttrs);\n";
	}
	stream << "}\n";

	return stream.str();
}


std::string GenerateAgentsNamesRelation(Model &model) {
	std::stringstream stream;
	// Add prototype
	stream << "void CreateAgentsNamesRelation(\n"
		"\tstd::unordered_map<AgentType, std::string> &agent_type_to_string,\n"
		"\tstd::unordered_map<std::string, AgentType> &string_to_agent_type) {\n";

	// Scan all agent types and build the two maps simultaneously
	for (const auto &agent : model.GetAgents()) {
		stream << "\tagent_type_to_string[" << agent.second.GetId() << "] = \"" << agent.first << "\";\n"
			   << "\tstring_to_agent_type[\"" << agent.first << "\"] = " << agent.second.GetId() << ";\n";
	}
	stream << "}\n";

	return stream.str();
}


std::string GenerateAttributesNamesRelation(Model &model) {
	std::stringstream stream;
	// Add prototype
	stream << "void CreateAttributesNamesRelation(\n"
		"\tAttributesNames &attribute_to_string,\n"
		"\tAttributesIds &string_to_attribute) {\n";

	// Scan all attributes and build the two maps simultaneously
	for (const auto &agent : model.GetAgents()) {
		for (const auto &attribute : agent.second.GetFields()) {
			stream << "\tattribute_to_string[std::make_pair("
				   << agent.second.GetId() << ", " << attribute.second.GetId()
				   << ")] = \"" << attribute.first << "\";\n"
				   << "\tstring_to_attribute[std::make_pair(\""
				   << agent.first << "\", \"" << attribute.first << "\")] = std::make_pair("
				   << agent.second.GetId() << ", " << attribute.second.GetId() << ");\n";
		}
	}
	stream << "}\n";

	return stream.str();
}


std::string GenerateNbAgentTypesFunction(Model &model) {
	std::stringstream stream;

	stream << "AgentType NbAgentTypes() {\n"
		   << "\treturn " << model.GetAgents().size() << ";\n"
		   << "}\n";

	return stream.str();
}


std::string GenerateNbInteractionTypesFunction(Model &model) {
	std::stringstream stream;

	stream << "InteractionType NbInteractionTypes() {\n"
		   << "\treturn " << model.GetInteractions().size() << ";\n"
		   << "}\n";

	return stream.str();
}


std::string GenerateStructFile(Model &model) {
	std::stringstream stream;

	stream << "#ifndef SIMULATION_STRUCTS_HPP_\n"
		   << "#define SIMULATION_STRUCTS_HPP_\n\n"
		   << "#include <vector>" << "\n"
		   << "#include \"types.hpp\"" << "\n"
		   << "#include \"agent.hpp\"" << "\n"
		   << "#include \"" << model.GetModelFileName() << "\"\n"
		   << "#include \"interaction.hpp\"" << "\n\n"
		   << GenerateAttributesStruct(model) << "\n"
		   << "#endif\n";

	return stream.str();
}


std::string GenerateMasterInitialization(Model &model, clang::ASTContext *context) {
	std::stringstream stream;

	stream << "#include \""
		   << model.GetModelFileName() << "\"" << "\n"
		   << "#include <vector>" << "\n"
		   << "#include \"simulation_structs.hpp\"\n"
		   << "#include \"types.hpp\"" << "\n\n";

	stream << GenerateAttributesMPIDatatypesFunction(model, context) << "\n"
		   << GenerateAgentsMPIDatatypesFunction(model) << "\n"
		   << GenerateCriticalStructsMPIDatatypesFunction(model) << "\n"
		   << GenerateInteractionsMPIDatatypesFunction(model, context) << "\n"
		   << GenerateAttributesSizeFunction(model) << "\n"
		   << GenerateCriticalAttributesFunction(model) << "\n"
		   << GenerateNonSendableAgentTypesFunction(model) << "\n"
		   << GeneratePublicAttributesOffsetsFunction(model) << "\n"
		   << GeneratePublicStructSizesFunction(model) << "\n"
		   << GenerateCriticalAttributesOffsetsFunction(model) << "\n"
		   << GenerateCriticalStructSizesFunction(model) << "\n"
		   << GenerateAgentsNamesRelation(model) << "\n"
		   << GenerateAttributesNamesRelation(model) << "\n"
		   << GenerateNbAgentTypesFunction(model) << "\n"
		   << GenerateNbInteractionTypesFunction(model) << "\n";

	return stream.str();
}
