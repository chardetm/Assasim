#include <sstream>
#include <string>

#include "generate_compilable_code.hpp"


std::string GenerateAgentConstructor(Model &model) {
	std::stringstream stream;
	// Generate an implementation for each agent type
	for (const auto &agent : model.GetAgents()) {
		stream << agent.first << "::" << agent.first
		       << "(AgentId id, AgentType type, MasterId master_id, Master& master,\n\t";
		for (const auto &field : agent.second.GetFields()) {
			if (field.second.IsSendable()) {
				stream << GetTypeAsString(field.second.GetType()) << " "
				       << field.first << "_i, ";
			}
		}
		stream.seekp(-2, std::ios_base::cur);
		stream << ") :\n"
		       << "Agent{id, type, master_id, master}, ";
		for (const auto &field : agent.second.GetFields()) {
			if (field.second.IsSendable()) {
				stream << field.first << "{" << field.first << "_i}, ";
			}
		}
		stream.seekp(-2, std::ios_base::cur);
		stream << "\n\t{}\n\n";
	}
	return stream.str();
}


std::string GenerateAgentReceiveMessage(Model &model) {
	std::stringstream stream;
	// Generate the code which will be used for each agent type
	std::stringstream pattern_stream;
	pattern_stream << "::ReceiveMessage(std::unique_ptr<Interaction> &inter) {\n"
	               << "\tswitch (inter->GetType()) {\n";
	for (const auto &interaction : model.GetInteractions()) {
		pattern_stream << "\t\tcase " << interaction.second.GetId() << ": {\n"
					   << "\t\t\t" << interaction.first << " *i = static_cast<" << interaction.first << "*>(inter.get());\n"
		               << "\t\t\treceived_" << interaction.first << ".push_back(*i);\n"
					   << "\t\t\t" << interaction.first << "MessageStruct *copied_struct =\n"
					   << "\t\t\t\tutils::malloc_construct<" << interaction.first
					        << "MessageStruct>(*static_cast<" << interaction.first << "MessageStruct*>(i->GetStructure()));\n"
					   << "\t\t\treceived_" << interaction.first << ".back().SetStructure(copied_struct);\n"
			           << "\t\t\tbreak;\n\t\t}\n";
	}
	pattern_stream << "\t\tdefault:\n\t\t\treturn;\n\t}\n}\n\n";
	std::string pattern_string = pattern_stream.str();
	// Generate the method for each agent type
	for (const auto &agent : model.GetAgents()) {
		stream << "void " << agent.first << pattern_string;
	}
	return stream.str();
}


std::string GenerateAgentResetMessages(Model &model) {
	std::stringstream stream;
	// Generate the code which will be used for each agent type
	std::stringstream pattern_stream;
	pattern_stream << "::ResetMessages() {\n";
	for (const auto &interaction : model.GetInteractions()) {
		pattern_stream << "\treceived_" << interaction.first << ".clear();\n";
	}
	pattern_stream << "}\n\n";
	std::string pattern_string = pattern_stream.str();
	// Generate the method for each agent type
	for (const auto &agent : model.GetAgents()) {
		stream << "void " << agent.first << pattern_string;
	}
	return stream.str();
}


std::string GenerateAgentGetPointerToAttribute(Model &model) {
	std::stringstream stream;
	// Generate an implementation for each agent type
	for (const auto &agent : model.GetAgents()) {
		stream << "void* " << agent.first << "::GetPointerToAttribute(Attribute attr) {\n"
			   << "\tswitch (attr) {\n";
		for (const auto &field : agent.second.GetFields()) {
			stream << "\t\tcase " << field.second.GetId() << ": {\n"
				   << "\t\t\treturn &" << field.first << ";\n"
				   << "\t\t\tbreak;\n\t\t}\n";
		}
		stream << "\t\tdefault:\n\t\t\treturn nullptr;\n"
			   << "\t}\n}\n\n";
	}
	return stream.str();
}


std::string GenerateAgentSetAttributeValue(Model &model) {
	std::stringstream stream;
	// Generate an implementation for each agent type
	for (const auto &agent : model.GetAgents()) {
		stream << "void " << agent.first << "::SetAttributeValue(Attribute attr, void* location) {\n"
			   << "\tswitch (attr) {\n";
		for (const auto &field : agent.second.GetFields()) {
			stream << "\t\tcase " << field.second.GetId() << ": {\n"
				   << "\t\t\tmemcpy(&" << field.first << ", location, sizeof("
				   << GetTypeAsString(field.second.GetType()) << "));\n"
				   << "\t\t\tbreak;\n\t\t}\n";
		}
		stream << "\t\tdefault:\n\t\t\treturn;\n"
			   << "\t}\n}\n\n";
	}
	return stream.str();
}


std::string GenerateAgentCheckModifiedCriticalAttributes(Model &model) {
	std::stringstream stream;
	// Generate an implementation for each agent type
	for (const auto &agent : model.GetAgents()) {
		stream << "void " << agent.first << "::CheckModifiedCriticalAttributes() {\n"
		       << "\tvoid* current_attribute;\n";
		for (const auto &field : agent.second.GetFields()) {
			if (field.second.IsCritical()) {
				stream << "\tcurrent_attribute = AskAttribute(" << field.second.GetId() << ", id_, type_);\n"
				       << "\tif (memcmp(&" << field.first << ", current_attribute, sizeof("
					                << GetTypeAsString(field.second.GetType()) << ")) != 0)\n"
				       << "\t\tupdated_critical_attributes_.push_back(" << field.second.GetId() << ");\n";
			}
		}
		stream << "}\n\n";
	}
	return stream.str();
}


std::string GenerateAgentCopyPublicAttributes(Model &model) {
	std::stringstream stream;
	// Generate an implementation for each agent type
	for (const auto &agent : model.GetAgents()) {
		stream << "void " << agent.first << "::CopyPublicAttributes(void *begin) {\n"
			   << "\t" << agent.first << "PublicAttrs public_structure;\n";
		for (const auto &field : agent.second.GetFields()) {
			if (field.second.GetAccess() == clang::AS_public && !field.second.IsCritical()) {
				stream << "\tpublic_structure." << field.first << " = " << field.first << ";\n";
			}
		}
		stream << "\tmemcpy(begin, &public_structure, sizeof(" << agent.first << "PublicAttrs" << "));\n"
			   << "}\n\n";
	}
	return stream.str();
}


std::string GenerateAgentCopyCriticalAttributes(Model &model) {
	std::stringstream stream;
	// Generate an implementation for each agent type
	for (const auto &agent : model.GetAgents()) {
		stream << "void " << agent.first << "::CopyCriticalAttributes(void *begin) {\n"
			   << "\t" << agent.first << "CriticalAttrs critical_structure;\n";
		for (const auto &field : agent.second.GetFields()) {
			if (field.second.IsCritical()) {
				stream << "\tcritical_structure." << field.first << " = " << field.first << ";\n";
			}
		}
		stream << "\tmemcpy(begin, &critical_structure, sizeof(" << agent.first << "CriticalAttrs" << "));\n"
			   << "}\n\n";
	}
	return stream.str();
}


std::string GenerateAgentFromStruct(Model &model) {
	std::stringstream stream;
	// Generate the prototype
	stream << "std::unique_ptr<Agent> Agent::FromStruct(void *agent_struct, MasterId master_id, Master &master) {\n"
		   << "\tswitch (((AgentStruct*)agent_struct)->type) {\n";
	for (const auto &agent : model.GetAgents()) {
		stream << "\t\tcase " << agent.second.GetId() << ": {\n";
		// Get the data
		stream << "\t\t\t" << agent.first << "Attrs *attrs = &("
			   << "(" << agent.first << "MessageStruct*) agent_struct)->data;\n";
		// Get a new instance of an agent of the good type
		stream << "\t\t\treturn std::make_unique<" << agent.first << ">("
			   << "((AgentStruct*)agent_struct)->id, ((AgentStruct*)agent_struct)->type, master_id, master\n"
			   << "\t\t\t,";
		// Add parameters for call to complete constructor
		for (const auto field : agent.second.GetFields()) {
			if (field.second.IsSendable()) {
				stream << "attrs->" << field.first << ",";
			}
		}
		stream.seekp(-1, std::ios_base::cur);
		stream << ");\n\t\t\tbreak;\n\t\t}\n";
	}
	stream << "\t\tdefault:\n";
	stream << "\t\t\treturn nullptr;\n";
	stream << "\t}\n";
	stream << "}\n";

	return stream.str();
}


std::string GenerateAgentCreateStruct(Model &model) {
	std::stringstream stream;
	// Generate an implementation for each agent type
	for (const auto &agent : model.GetAgents()) {
		stream << "void " << agent.first << "::CreateStruct() {\n";
		stream << "\t" << agent.first << "MessageStruct *agent_struct = "
			   << "utils::malloc_construct<" << agent.first << "MessageStruct>();\n";
		stream << "\tagent_struct->id = id_;\n"
			   << "\tagent_struct->type = type_;\n";
		for (const auto &field : agent.second.GetFields()) {
			if (field.second.IsSendable())
				stream << "\tagent_struct->data." << field.first << " = "
					   << field.first << ";\n";
		}
		stream << "\tstructure_ = (void*)agent_struct;\n"
			   << "}\n";
	}
	return stream.str();
}


inline std::string indent (unsigned nb) {
	return std::string(nb, '\t');
}



void GenerateWriteValue(std::ostream &stream, const std::string &datalocation, const std::string &fieldname, const std::string &jsonnode, const clang::QualType& clangcanonicaltype, unsigned i) {
	static unsigned long long uniqueid = 0;
	unsigned long long myid = uniqueid++;
	if (clangcanonicaltype.getTypePtr()->isStructureType()) {
		stream << indent(i) << "Value attribute" << myid << ";\n";
		clang::RecordDecl* struct_decl = clangcanonicaltype.getTypePtr()->getAsStructureType()->getDecl();
		for (const auto* field : struct_decl->fields()) {
			GenerateWriteValue(stream, datalocation + "." + fieldname, field->getName().str(), "attribute" + std::to_string(myid), field->getType().getCanonicalType(), i);
		}
		stream << indent(i) << jsonnode << "[\"" << fieldname << "\"] = std::move(attribute" << myid << ");\n";
	} else {
		stream << indent(i) << jsonnode << "[\"" << fieldname << "\"] = " << datalocation << "." << fieldname << ";\n";
	}
}



std::string GenerateAgentGetJsonNode(Model &model) {
	std::stringstream stream;
	for (const auto &agent : model.GetAgents()) {
		stream << "ubjson::Value " << agent.first << "::GetJsonNode() {\n";
		stream << "\tusing namespace ubjson;\n"
		       << "\tValue attributesNode;\n";
		for (const auto &field : agent.second.GetFields()) {
			if (field.second.IsSendable()) {
				GenerateWriteValue(stream, "(*this)", field.first, "attributesNode", field.second.GetType().getCanonicalType(), 1);
				/*
				//FIXME: Remove this
				if (field.second.GetType().getCanonicalType().getTypePtr()->isStructureType()) {
					//TODO: handle structures
					//FIXME: remove this
					stream << "\t// Skipped structure " << field.first << "\n";
					continue;
				}
				stream << "\t{\n"
				       << "\t\tValue attribute;\n"
				       << "\t\tattribute[\"name\"] = \"" << field.first << "\";\n"
				       << "\t\tattribute[\"value\"] = this->" << field.first << ";\n"
				       << "\t\tattributesNode.push_back(std::move(attribute));\n"
				       << "\t}\n";
				*/
			}
		}
		stream << "\tValue agentNode;\n"
		       << "\tagentNode[\"id\"] = static_cast<unsigned long long>(id_);\n"
			   << "\tagentNode[\"attributes\"] = std::move(attributesNode);\n"
			   << "\treturn agentNode;\n"
			   << "}\n";
	}
	return stream.str();
}

std::string GenerateInteractionCreateStruct(Model &model) {
	std::stringstream stream;

	for (const auto &interaction : model.GetInteractions()) {
		stream << "void " << interaction.first << "::CreateStruct() {\n";

		// Initialize structure_ of interaction
		stream << "\t" << interaction.first << "MessageStruct *interaction_s = "
			   << "utils::malloc_construct<" << interaction.first << "MessageStruct>();\n";
		stream << "\tinteraction_s->type = type_;\n"
			   << "\tinteraction_s->sender_id = sender_id_;\n"
			   << "\tinteraction_s->sender_type = sender_type_;\n"
			   << "\tinteraction_s->recipient_id = recipient_id_;\n"
			   << "\tinteraction_s->recipient_type = recipient_type_;\n";
		for (const auto &field : interaction.second.GetFields()) {
			if (field.second.IsSendable())
				stream << "\tinteraction_s->data." << field.first << " = "
					   << field.first << ";\n";
		}
		stream << "\tSetStructure((void*)interaction_s);\n";
		stream << "}\n";
	}

	return stream.str();
}

std::string GenerateInteractionFromStruct(Model &model) {
	std::stringstream stream;
	// Generate the prototype
	stream << "std::unique_ptr<Interaction> Interaction::FromStruct(void *interaction_struct) {\n"
		   << "\tInteractionStruct* message_attrs = (InteractionStruct*)interaction_struct;\n"
		   << "\tswitch (message_attrs->type) {\n";
	for (const auto &interaction : model.GetInteractions()) {
		// Get the data
		stream << "\t\tcase " << interaction.second.GetId() << ": {\n";
		stream << "\t\t\t" << interaction.first << "Attrs *attrs = &("
			   << "(" << interaction.first << "MessageStruct*) interaction_struct)->data;\n";
		// Get a new instance of an agent of the good type
		stream << "\t\t\tstd::unique_ptr<Interaction> interaction(new " << interaction.first << "("
			   << "message_attrs->type,"
			   << "message_attrs->sender_id,"
			   << "message_attrs->sender_type,"
			   << "message_attrs->recipient_id,"
			   << "message_attrs->recipient_type\n"
			   << "\t\t\t,";
		// Add parameters for call to complete constructor
		for (const auto field : interaction.second.GetFields()) {
				stream << "attrs->" << field.first << ",";
		}
		stream.seekp(-1, std::ios_base::cur);
		stream << "));\n";
		stream << "\t\t\treturn interaction;\n\t\t}\n";
	}
	stream << "\t\tdefault:\n";
	stream << "\t\t\treturn nullptr;\n";
	stream << "\t}\n";
	stream << "}\n";

	return stream.str();
}

void AddReceivedInteractionsInAgents(Model &model, clang::Rewriter &rewriter) {
	std::stringstream stream;
	stream << "private:" << std::endl;
	for (const auto &interaction : model.GetInteractions()) {
		stream << "\tstd::vector<" << interaction.first
			   << "> received_" << interaction.first
			   << ";" << std::endl;
	}

	for (const auto &agent : model.GetAgents()) {
		rewriter.InsertText(agent.second.GetDecl()->getLocEnd(),stream.str(),true,true);
	}
}

void AddConstructorsInInteractionsStep2(Model &model, clang::Rewriter &rewriter) {
	for (const auto &interaction : model.GetInteractions()) {
		std::stringstream stream;

		stream << "private:\n";
		// Add the function CreateStruct called in the constructors
		stream << "\tvoid CreateStruct();";

		stream << "public:\n"
			   << "\t" << interaction.first << "(const " << interaction.first << " &e) : Interaction(e) {}\n"
			   << "\tvoid operator=(const " << interaction.first << " &e) {Interaction::operator=(e);}\n";

		clang::Rewriter::RewriteOptions rewrite_options;
		rewrite_options.RemoveLineIfEmpty = true;

		const clang::CXXRecordDecl *decl = interaction.second.GetDecl();
		for (const auto *ctor : decl->ctors()) {
			if (ctor->isCopyOrMoveConstructor() || ctor->isCopyAssignmentOperator())
				continue;
			stream << "\t" << interaction.first << "("
				   << "uint64_t type_p, uint64_t sender_id_p, uint64_t sender_type_p, "
				   << "uint64_t recipient_id_p, uint64_t recipient_type_p,";

			for (const auto *param : ctor->params()) {
				std::string s;
				llvm::raw_string_ostream param_code(s);
				param->print(param_code);
				stream << param_code.str()
					   << ",";
			}
			stream.seekp(-1,std::ios_base::cur);

			stream << ") : \n\tInteraction(type_p,sender_id_p,sender_type_p,recipient_id_p,recipient_type_p), ";

			clang::LangOptions lang_options;
			clang::PrintingPolicy policy(lang_options);

			unsigned k = 0;
			for (const auto *init : ctor->inits()) {
				std::string s;
				llvm::raw_string_ostream init_expr(s);

				init->getInit()->printPretty(init_expr, nullptr, policy);
				if (init->getMember() != nullptr) {
					stream << init->getMember()->getNameAsString() << "(" << init_expr.str() << "), ";
					k++;
				}
			}
			stream.seekp(-2,std::ios_base::cur);
			stream << " ";
			std::string s;
			llvm::raw_string_ostream ctor_body(s);

			ctor->getBody()->printPretty(ctor_body, nullptr, policy);
			std::string body = std::string(ctor_body.str());

			int i = body.length()-1;
			while(i >= 0 && body[i] != '}')
				i--;

			stream << " " << body.substr(0,i) <<"\tCreateStruct();}\n";

			// Delete the user-specified constructor
			rewriter.RemoveText(clang::SourceRange(ctor->getLocStart(),ctor->getLocEnd()), rewrite_options);
		}
		stream << "\n";

		rewriter.InsertText(decl->getLocEnd(), stream.str(), true, true);
	}
}

void AddPrototypesInAgentsStep2(Model &model, clang::Rewriter &rewriter) {
	for (const auto &agent : model.GetAgents()) {
		std::stringstream stream;

		// First add the prototype for the complete constructor
		stream << "public:\n";
		stream << "\t" << agent.first
		       << "(AgentId id, AgentType type, MasterId master_id, Master& master, ";
		for (const auto &field : agent.second.GetFields()) {
			if (field.second.IsSendable()) {
				stream << GetTypeAsString(field.second.GetType()) << " "
				       << field.first << "_i, ";
			}
		}
		stream.seekp(-2, std::ios_base::cur);
		stream << ");\n";

		// Then add the prototypes of the abstract functions defined in class Agent
		stream << "private:\n";

		stream << "\tvoid " << "ReceiveMessage(std::unique_ptr<Interaction> &inter);\n"
			   << "\tvoid " << "ResetMessages();\n"
			   << "\tvoid* " << "GetPointerToAttribute(Attribute attr);\n"
			   << "\tvoid " << "SetAttributeValue(Attribute attr, void* location);\n"
			   << "\tvoid " << "CheckModifiedCriticalAttributes();\n"
			   << "\tvoid " << "CopyPublicAttributes(void *begin);\n"
			   << "\tvoid " << "CopyCriticalAttributes(void *begin);\n"
			   << "\tvoid " << "CreateStruct();\n"
			   << "\tubjson::Value " << "GetJsonNode();\n";

		rewriter.InsertText(agent.second.GetDecl()->getLocEnd(), stream.str(), true, true);
	}
}

std::string GenerateAgentCpp(Model &model) {
	std::stringstream stream;

	stream << "#include <cstring>\n"
	       << "#include \"types.hpp\"\n"
		   << "#include \"agent.hpp\"\n"
		   << "#include \"simulation_structs.hpp\"\n"
		   << "#include \"utils/memory.hpp\"\n"
		   << "#include \"" << model.GetModelFileName() << "\"\n\n";
	stream << GenerateAgentConstructor(model) << "\n"
		   << GenerateAgentReceiveMessage(model) << "\n"
	       << GenerateAgentResetMessages(model) << "\n"
		   << GenerateAgentGetPointerToAttribute(model) << "\n"
		   << GenerateAgentSetAttributeValue(model) << "\n"
		   << GenerateAgentCheckModifiedCriticalAttributes(model) << "\n"
		   << GenerateAgentCopyPublicAttributes(model) << "\n"
	       << GenerateAgentCopyCriticalAttributes(model) << "\n"
		   << GenerateAgentFromStruct(model) << "\n"
		   << GenerateInteractionCreateStruct(model) << "\n"
		   << GenerateInteractionFromStruct(model) << "\n"
		   << GenerateAgentCreateStruct(model) << "\n"
		   << GenerateAgentGetJsonNode(model) << "\n";
	return stream.str();
}


void GenerateReadValue(std::ostream &stream, const std::string &datalocation, const std::string &fieldname, const std::string &jsonvalue, const clang::QualType& clangcanonicaltype, unsigned i) {
	static unsigned long long uniqueid = 0;
	std::string cast = GetTypeAsString(clangcanonicaltype);
	const clang::Type* agentTypePtr = clangcanonicaltype.getTypePtr();
	std::string jsontype;

	if (agentTypePtr->isStructureType()) {
		stream << indent(i) << "} else if (" << jsonvalue << ".first == \"" << fieldname << "\") {\n";
		unsigned long long myid = uniqueid++;
		stream << indent(i+1) << "for (auto &value" << myid << " : " << jsonvalue << ".second.as<json_map>()) {\n"
		       << indent(i+2) << "if (false) {\n";
		clang::RecordDecl* struct_decl = clangcanonicaltype.getTypePtr()->getAsStructureType()->getDecl();
		for (const auto* field : struct_decl->fields()) {
			GenerateReadValue(stream, datalocation + "." + fieldname, field->getName().str(), "value" + std::to_string(myid), field->getType().getCanonicalType(), i+2);
		}
		stream << indent(i+2) << "}\n"
		       << indent(i+1) << "}\n";
	} else {
		stream << indent(i) << "} else if (" << jsonvalue << ".first == \"" << fieldname << "\") {\n"
		       << indent(i+1) << "json_value temp_json(" << jsonvalue << ".second);\n";
		if (agentTypePtr->isBooleanType()) {
			stream << indent(i+1) << "if (temp_json.get_type() == json_value::type::boolean) {\n"
			       << indent(i+2) << datalocation << "." << fieldname << " = static_cast<" << cast << ">(temp_json);\n"
			       << indent(i+1) << "} else {\n"
			       << indent(i+2) << datalocation << "." << fieldname << " = static_cast<" << cast << ">(" << jsonvalue << ".second.as<json_int>());\n"
			       << indent(i+1) << "}\n";
		} else if (agentTypePtr->isAnyCharacterType()) {
			//TODO: Check how it behaves with wide characters
			//FIXME: Check that the string has at least length 1
			stream << indent(i+1) << "if (temp_json.get_type() == json_value::type::string) {\n"
			       << indent(i+2) << "if (static_cast<std::string>(temp_json).empty()) {\n"
			       << indent(i+3) << "std::cerr << \"Warning: trying to initialize character " + datalocation + "." + fieldname << " with an empty string ; one character required in the string.\";\n"
			       << indent(i+3) << datalocation << "." << fieldname << " = static_cast<" << cast << ">('0');\n"
			       << indent(i+2) << "}\n"
			       << indent(i+2) << datalocation << "." << fieldname << " = static_cast<" << cast << ">(static_cast<std::string>(temp_json).at(0));\n"
			       << indent(i+1) << "} else {\n"
			       << indent(i+2) << datalocation << "." << fieldname << " = static_cast<" << cast << ">(" << jsonvalue << ".second.as<json_int>());\n"
			       << indent(i+1) << "}\n";
			
		} else if (agentTypePtr->isEnumeralType()) {
			stream << indent(i+1) << datalocation << "." << fieldname << " = static_cast<" << cast << ">(" << jsonvalue << ".second.as<json_int>());\n";
		} else if (agentTypePtr->isIntegerType()) {
			stream << indent(i+1) << datalocation << "." << fieldname << " = static_cast<" << cast << ">(" << jsonvalue << ".second.as<json_int>());\n";
		} else if (agentTypePtr->isFloatingType()) {
			stream << indent(i+1) << "if (" << jsonvalue << ".second.get_type() == json_value::type::integer) {\n"
			       << indent(i+2) << datalocation << "." << fieldname << " = static_cast<" << cast << ">(" << jsonvalue << ".second.as<json_int>());\n"
			       << indent(i+1) << "} else {\n"
			       << indent(i+2) << datalocation << "." << fieldname << " = static_cast<" << cast << ">(" << jsonvalue << ".second.as<json_float>());\n"
			       << indent(i+1) << "}\n";
		} else {
			WarningMessage() << "Logic error in the generation of Instanciate: GenerateReadValue got an invalid type of variable: " << cast << ".";
			return;
		}
	}
}


std::string GenerateUserInterfaceModelCpp(Model &model) {
	std::stringstream stream;

	stream << "#include <memory>\n"
	       << "#include <vector>\n"
	       << "#include <string>\n"
	       << "#include <stdexcept>\n"
	       << "#include \"master.hpp\"\n"
	       << "#include \"user_interface_model.hpp\"\n"
	       << "#include \"utils/memory.hpp\"\n"
	       << "#include \"simulation_structs.hpp\"\n"

	       << "#include \"libs/jeayeson/include/jeayeson/jeayeson.hpp\"\n"
	       << "#include \"libs/jeayeson/include/jeayeson/value.hpp\"\n"
	       << "\n";

	// Begining of the function which parses the input
	stream << "void ParseModelCommand(const char *buffer, std::unique_ptr<Master> &root_master, bool is_alive) {\n"
	       << "\tstd::istringstream input(buffer);\n"
	       << "\tstd::string command; input >> command;\n";

	// Command print_model
	stream << "\tif (command == \"print_model\") {\n";

	// Print the model_structure
	stream << "\t\tstd::cout << \"";

	model.PrintJson(stream,true);


	stream <<"\";\n";

	// Command print_agent
	stream << "\t} else if (command == \"print_agent\") {\n";


	stream << "\t}\n"
	       << "}\n\n";



	// Function std::vector<void*> Instanciate(std::string file);

	stream << "std::vector<void*> Instanciate(std::string file) try {\n"
	       << "\tstd::vector<void*> pointers;\n"
	       << "\tjson_map map{json_file{file}};\n"
	       << "\tfor (auto &type : map[\"agent_types\"].as<json_array>()) {\n"
	       << "\t\tauto start = pointers.size();\n"
	       << "\t\tstd::array<unsigned long long, " << model.GetAgents().size() << "> ids;\n"
		   << "\t\tids.fill(0);\n"
	       << "\t\tif (false) {\n";

	for (const auto &agent : model.GetAgents()) {
		stream << "\t\t} else if (type[\"type\"].as<std::string>() == \"" << agent.first << "\" && type[\"number\"].as<json_int>() > 0) {\n"
		       << "\t\t\tpointers.push_back(utils::malloc_construct<" << agent.first << "MessageStruct>());\n"
		       << "\t\t\tstatic_cast<" << agent.first << "MessageStruct*>(pointers.back())->id = ids.at(" << agent.second.GetId() << ")++;\n"
		       << "\t\t\tstatic_cast<" << agent.first << "MessageStruct*>(pointers.back())->type = " << agent.second.GetId() << ";\n"

		// Handling default values

		       << "\t\t\tif (type.as<json_map>().has(\"default_values\")) {\n"
		       << "\t\t\t\tfor (auto &attribute : type[\"default_values\"].as<json_map>()) {\n"
		       << "\t\t\t\t\tif (false) {\n";

		for (const auto &field : agent.second.GetFields()) {
			if (field.second.IsSendable()) {
				GenerateReadValue(stream, "static_cast<" + agent.first + "MessageStruct*>(pointers.at(start))->data", field.first, "attribute", field.second.GetType().getCanonicalType(), 5);
			}
		}
		stream << "\t\t\t\t\t}\n"
		       << "\t\t\t\t}\n"
			   << "\t\t\t}\n"

		// End default values

		// Now we use the copy constructor on the remaining agent structures to copy the default values
		       << "\t\t\tfor (int i=1; i<type[\"number\"].as<json_int>(); ++i) {\n"
		       << "\t\t\t\tpointers.push_back(utils::malloc_construct<" << agent.first << "MessageStruct>(*static_cast<" << agent.first << "MessageStruct*>(pointers.at(start))));\n"
		       << "\t\t\t\tstatic_cast<" << agent.first << "MessageStruct*>(pointers.back())->id = ids.at(" << agent.second.GetId() << ")++;\n"
		       << "\t\t\t\tstatic_cast<" << agent.first << "MessageStruct*>(pointers.back())->type = " << agent.second.GetId() << ";\n"
		       << "\t\t\t}\n"
		       << "\t\t\tif (!type.as<json_map>().has(\"agents\"))\n"
			   << "\t\t\t\tcontinue;\n"
		       << "\t\t\tfor (auto &agent : type[\"agents\"].as<json_array>()) {\n"
		       << "\t\t\t\tauto id = agent[\"id\"].as<json_int>();\n"
		       << "\t\t\t\tfor (auto &attribute : agent[\"attributes\"].as<json_map>()) {\n"
		       << "\t\t\t\t\tif (false) {\n";

		for (const auto &field : agent.second.GetFields()) {
			if (field.second.IsSendable()) {
				GenerateReadValue(stream, "static_cast<" + agent.first + "MessageStruct*>(pointers.at(id+start))->data", field.first, "attribute", field.second.GetType().getCanonicalType(), 5);
			}
		}
		stream << "\t\t\t\t\t}\n"
		       << "\t\t\t\t}\n"
		       << "\t\t\t}\n";
	}
	stream << "\t\t}\n"
	       << "\t}\n"
	       << "\treturn pointers;\n"
		   << "} catch (const std::exception& e) {\n"
		   << "\tthrow InstanciateException(e);\n"
		   << "} catch (...) {\n"
		   << "\tthrow InstanciateException(\"unknown error\");\n"
		   << "}\n\n";
	return stream.str();
}
