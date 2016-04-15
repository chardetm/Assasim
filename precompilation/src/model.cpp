/**
 * \file model.cpp
 * \brief Implements some methods of ClassTypeContainer and Model.
 */

#include <iostream>
#include <ios>
#include <sstream>
#include <fstream>
#include <unordered_map>

#include <cstring>

#include "analyze_class.hpp"
#include "utils.hpp"
#include "model.hpp"

#include "../simulation_basis/libs/ubjsoncpp/include/value.hpp"
#include "../simulation_basis/libs/ubjsoncpp/include/stream_writer.hpp"

extern std::unordered_set<PairLocation, hashPairLocation> CriticalLocation;
extern clang::Rewriter rewriter;


void ClassTypeContainer::AddFields(const clang::QualType &type) {
	const clang::CXXRecordDecl *declaration = ClassTypeContainer(type).GetDecl();
	// Store the fields of the class
	for (const auto *field : declaration->fields()) {
		if (field->getName() == "sender_id_" || field->getName() == "sender_type_" || field->getName().substr(0,9) == "received_")
			continue;
		index_fields_++;
		// Check if the field is defined with the critical tag
		clang::SourceLocation loc = field->getLocation();
		PairLocation pair_loc = PairLocation(rewriter.getSourceMgr().getFileID(loc), rewriter.getSourceMgr().getSpellingLineNumber(loc));

		fields_[field->getName()] = FieldTypeContainer(field->getType(),index_fields_, field->getAccess(), (CriticalLocation.count(pair_loc) == 1));
	}
	// Recursively store the fields of all the inherited classes
	for (const auto &base : declaration->bases())
		AddFields(base.getType());
}


bool AgentTypeContainer::HasBehavior() const {
	// Check if one of its bases has a Behavior method
	if (GetDecl() == nullptr) {
		return false;
	}
	for (const auto &base : GetDecl()->bases()) {
		if (AgentTypeContainer(base.getType(), 0, clang::FileID()).HasBehavior()) {
			return true;
		}
	}
	// Check if the class has a Behavior method which is not pure virtual
	for (const auto *method : GetDecl()->methods()) {
		if (method->isUserProvided() && IsTrueBehavior(method) && !(method->isVirtual() && method->isPure())) {
			return true;
		}
	}
	return false;
}


std::string AgentTypeContainer::AttributesStruct(const std::string &name) const {
	std::stringstream stream;
	stream << "struct " << name << "Attrs" << " {\n";
	for (const auto &field : GetFields()) {
		if (!field.second.IsSendable())
			continue; // Ignore non sendable types
		stream <<  "\t" << GetTypeAsString(field.second.GetType().getCanonicalType())
			   << " " << field.first <<";\n";
	}
	stream << "};\n";

	//if (stream.str().length() != name.length() + 18) // If struct is not empty, return it
	return stream.str();
	//else
	//	return "";
}


std::string AgentTypeContainer::PublicAttributesStruct(const std::string &name) const {
	std::stringstream stream;
	stream << "struct " << name << "PublicAttrs" << " {\n";
	for (const auto &field : GetFields()) {
		if (field.second.GetAccess() == clang::AS_public && !field.second.IsCritical())
		stream <<  "\t" << GetTypeAsString(field.second.GetType().getCanonicalType())
			   << " " << field.first <<";\n";
	}
	stream << "};\n";

	//if (stream.str().length() != name.length() + 24) // If struct is not empty, return it
	return stream.str();
	//else
	//	return "";
}


std::string AgentTypeContainer::CriticalAttributesStruct(const std::string &name) const {
	std::stringstream stream;
	stream << "struct " << name << "CriticalAttrs" << " {\n";
	for (const auto &field : GetFields()) {
		if (field.second.GetAccess() == clang::AS_public && field.second.IsCritical())
		stream <<  "\t" << GetTypeAsString(field.second.GetType().getCanonicalType())
			   << " " << field.first <<";\n";
	}
	stream << "};\n";

	//if (stream.str().length() != name.length() + 26) // If struct is not empty, return it
	return stream.str();
	//else
	//	return "";
}


std::string AgentTypeContainer::MessageStruct(const std::string &name) const {
	std::stringstream stream;

	stream << "struct " << name << "MessageStruct {\n"
		   << "\tAgentId id;\n"
		   << "\tAgentType type;\n"
		   << "\t" << name << "Attrs data;\n"
		   << "};\n";

	return stream.str();
}


bool InteractionTypeContainer::HasCompleteConstructor() const {
	for (const auto *ctor : GetDecl()->ctors()) {
		if ((size_t)std::distance(ctor->init_begin(),ctor->init_end()) == GetFields().size())
			return true;
	}
	return false;
}

std::string InteractionTypeContainer::AttributesStruct(const std::string &name) const {
	std::stringstream stream;
	stream << "struct " << name << "Attrs" << " {\n";
	for (const auto &field : GetFields()) {
		stream <<  "\t" << GetTypeAsString(field.second.GetType().getCanonicalType())
			   << " " << field.first <<";\n";
	}
	stream << "};\n";

	//if (stream.str().length() != name.length() + 18) // If struct is not empty, return it
	return stream.str();
	//else
	//	return "";
}


std::string InteractionTypeContainer::MessageStruct(const std::string &name) const {
	std::stringstream stream;

	stream << "struct " << name << "MessageStruct {\n"
		   << "\tInteractionType type;\n"
		   << "\tAgentId sender_id;\n"
		   << "\tAgentType sender_type;\n"
		   << "\tAgentId recipient_id;\n"
		   << "\tAgentType recipient_type;\n"
		   << "\t" << name << "Attrs data;\n"
		   << "};\n";

	return stream.str();
}


ubjson::Value TypeJsonNode (const clang::QualType& type) {
	using namespace ubjson;

	Value node;

	// If it is an anonymous structure, print all the fields recursively
	if (type.getTypePtr()->isStructureType()) {
		node["kind"] = "struct";

		Value structContent;
		clang::RecordDecl* struct_decl = type.getTypePtr()->getAsStructureType()->getDecl();

		// Print the types of all the fields
		for (const auto& field : struct_decl->fields()) {
			Value structField;
			structField["type"] = TypeJsonNode(field->getType().getCanonicalType());
			structField["name"] = field->getName().str();
			structContent.push_back(std::move(structField));
		}
		node["content"] = std::move(structContent);
	} else if (type.getTypePtr()->isBooleanType()) {
		node["kind"] = "builtin";
		node["cpptype"] = "bool";
	} else {
		node["kind"] = "builtin";
		node["cpptype"] = type.getAsString(); //just get the type name
	}
	return node;
}


ubjson::Value GetJson(const AgentTypeMemory& agents, const InteractionTypeMemory& interactions) {
	using namespace ubjson;

	//std::ofstream output(file, std::binary);
	//StreamWriter<std::ostream> writer(output);
	Value root;

	Value agent_types;
	for (const auto &pair : agents) {
		Value agent_t;
		agent_t["name"] = pair.first;
		agent_t["sendable"] = pair.second.IsSendable() ? 1 : 0;
		Value attributes;
		for (const auto &field : pair.second.GetFields()) {
			Value attribute;
			attribute["visibility"] = field.second.IsCritical() ?
				"critical" :
				(field.second.GetAccess() == clang::AS_public ? "public" : "private");
			attribute["type"] = TypeJsonNode(field.second.GetType().getCanonicalType());
			attribute["name"] = field.first;
			attributes.push_back(std::move(attribute));
		}
		agent_t["attributes"] = std::move(attributes);
		agent_types.push_back(std::move(agent_t));
	}
	root["agent_types"] = std::move(agent_types);

	Value interaction_types;
	for (const auto &pair : interactions) {
		Value interaction_t;
		interaction_t["name"] = pair.first;
		Value attributes;
		for (const auto &field : pair.second.GetFields()) {
			Value attribute;
			attribute["type"] = TypeJsonNode(field.second.GetType());
			attribute["name"] = field.first;
			attributes.push_back(std::move(attribute));
		}
		interaction_t["attributes"] = std::move(attributes);
		interaction_types.push_back(std::move(interaction_t));
	}
	root["interaction_types"] = std::move(interaction_types);
	return root;
}


bool Model::WriteBinaryJson(const std::string &file) const {
	using namespace ubjson;
	Value json = GetJson(agents_, interactions_);

	std::ofstream output{file, std::ios::binary};
	StreamWriter<std::ostream> writer(output);

	auto result = writer.writeValue(json);
	return result.second;
}


std::ostream& Model::PrintJson(std::ostream &ost, bool to_string) const {
	using namespace ubjson;
	Value json = GetJson(agents_, interactions_);
	if (!to_string) {
		ost << to_ostream(json, to_ostream::pretty) << std::endl;
		return ost;
	}

	std::stringstream stream;
	stream << to_ostream(json, to_ostream::pretty);

	char c;
	while (stream >> c) {
		if (c == '\"')
			ost << "\\\"";
		else
			ost << c;
	}

	return ost;
}


inline std::string indent (unsigned nb) {
	return std::string(nb, '\t');
}


std::ostream& TypeEmptyInstance (std::ostream& ost, const clang::QualType& type, unsigned i) {
	using namespace ubjson;

	// If it is an anonymous structure, print all the fields recursively
	if (type.getTypePtr()->isStructureType()) {
		ost << "{";

		bool first{true};
		Value structContent;
		clang::RecordDecl* struct_decl = type.getTypePtr()->getAsStructureType()->getDecl();

		// Print the types of all the fields
		for (const auto* field : struct_decl->fields()) {
			if (!first) {
				ost << ',';
			} else {
				first = false;
			}
			std::ostringstream ststream;
			TypeEmptyInstance(ststream, field->getType().getCanonicalType(), i+1);
			ost << std::endl
			    << indent(i) << "\"" << field->getName().str() << "\": " << ststream.str();
		}
		ost << std::endl << indent(i) << "}";
	} else {
		ost << "#";
	}
	return ost;
}


std::ostream& GetEmptyInstance(std::ostream& ost, const AgentTypeMemory& agents) {
	using namespace ubjson;

	ost << "{" << std::endl
	    << "\t\"agent_types\": [";

	bool first{true};

	for (const auto &pair : agents) {
		if (!first) {
			ost << ',';
		} else {
			first = false;
		}
		ost << std::endl
		    << "\t\t{" << std::endl
			<< "\t\t\t\"type\": \"" << pair.first << "\"," << std::endl
			<< "\t\t\t\"number\" : #," << std::endl
			<< "\t\t\t\"default_values\": {";
		Value attributes;

		bool first2{true};
		for (const auto &field : pair.second.GetFields()) {
			Value attribute;
			if (field.second.IsSendable()) {
				if (!first2) {
					ost << ',';
				} else {
					first2 = false;
				}
				std::ostringstream ststream;
				TypeEmptyInstance(ststream, field.second.GetType().getCanonicalType(), 5);
				ost << std::endl
				    << "\t\t\t\t\"" << field.first << "\": " << ststream.str();
			}
		}
		ost << std::endl
		    << "\t\t\t}," << std::endl
		    << "\t\t\t\"agents\": [" << std::endl
		    << "\t\t\t\t{" << std::endl
		    << "\t\t\t\t\t\"id\": #," << std::endl
		    << "\t\t\t\t\t\"attributes\": {" << std::endl
		    << "\t\t\t\t\t}" << std::endl
		    << "\t\t\t\t}" << std::endl
		    << "\t\t\t]" << std::endl
		    << "\t\t}";
	}

	ost << std::endl
	    << "\t]" << std::endl
	    << "}" << std::endl;

	return ost;
}


std::ostream& Model::PrintEmptyInstance(std::ostream &ost) const {
	return GetEmptyInstance(ost, agents_);
}
