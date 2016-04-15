#include <cstring>
#include <map>
#include <sstream>

#include "utils.hpp"
#include "analyze_class.hpp"
#include "model.hpp"
#include "build_model.hpp"
#include "model_environment.hpp"


void AddConstructorInInteraction(Model &model, clang::Rewriter &rewriter) {
    for (const auto &interaction : model.GetInteractions()) {
        std::stringstream stream;
		// Add the name of the constructor
		stream << "public:" << std::endl
			   << "\t" << interaction.first << "(";
		// Add the arguments in the form: field_type [field_name]_p
		for (const auto &field : interaction.second.GetFields())
			stream << GetTypeAsString(field.second.GetType().getCanonicalType()) << " " << field.first << "_p, ";
		if (interaction.second.GetFields().size() > 0) // Remove the last comma
			stream.seekp(-2,std::ios_base::cur);
		stream << ")";
		if (interaction.second.GetFields().size() > 0)
			stream << " : ";
		// Add the assignments
		for (const auto &field : interaction.second.GetFields())
			stream << " " << field.first << "(" << field.first << "_p),";
		if (interaction.second.GetFields().size() > 0)
				stream.seekp(-1, std::ios_base::cur);
		stream << " {}\n";
		// Write the constructor in the definition
        rewriter.InsertText(interaction.second.GetDecl()->getLocEnd(), stream.str(), true, true);
    }
}

/// \deprecated This function has no purpose anymore.
void AddGetterInAgents(Model &model, clang::Rewriter &rewriter) {
    for (const auto &agent : model.GetAgents()) {
        std::stringstream stream;
        stream << "\npublic:\n";
        for (const auto &field : agent.second.GetFields()) {
            //if the attribute is a public one
            if (field.second.GetAccess() == clang::AS_public) {
                std::string type = GetTypeAsString(field.second.GetType());
                std::string name = field.first;
                // Add the getter
                stream << "\tconst " << type << " &Get_" << name << "() {\n"
                      << "\t\treturn " << name << ";\n"
                      << "\t}\n";
            }
        }
        rewriter.InsertText(agent.second.GetDecl()->getLocEnd(), stream.str(), true, true);
    }
}

void AddBehaviorPrototypeInAgents(Model &model, clang::Rewriter &rewriter) {
	std::stringstream stream;
	stream << "public:" << std::endl
		   << "\tvoid Behavior();" << std::endl;
	
	for (const auto &agent : model.GetAgents()){
		if (!agent.second.HasBehavior())
			rewriter.InsertText(agent.second.GetDecl()->getLocEnd(), stream.str(), true, true);
		else {
			// This is a magic trick to export the main file if unchanged
			rewriter.InsertText(agent.second.GetDecl()->getLocEnd(), "", true, true); 		  }
	}
}

std::string GenerateBehaviorsContent(Model &model, clang::Rewriter &rewriter) {
	int i = 0;
	std::stringstream stream;
	
	std::string main_file_name = model.GetSourceManager()->getFileEntryForID(
					model.GetSourceManager()->getMainFileID())->getName();
	ExtractMainDirectory(main_file_name);
	
	stream << "#include \"consts.hpp\"" << std::endl
		   << "#include \"" << main_file_name << "\"" << std::endl << std::endl; 
	for (const auto &agent : model.GetAgents()) {
		i++;
		
		stream << "void " << agent.first <<  "::Behavior() try {" << std::endl << std::endl
			   << "\t/* Implement behavior of " << agent.first << " here */" << std::endl << std::endl
			   << "} catch (const std::exception &e) {" << std::endl
			   << "\tstd::cerr << \"[\" << TimeStep() << \"]\" << \" In agent " << agent.first << "\" << id_ << \": \" << e.what() << std::endl;" << std::endl
			   << "} catch (...) {}" << std::endl << std::endl;
	}
	return stream.str();
}


std::string GenerateAgentHeaderContent(Model &model) {
	std::stringstream stream;
	// Preprocessor
	stream << "#ifndef AGENT_HPP_" << std::endl
		   << "#define AGENT_HPP_" << std::endl
		   << "#include <vector>" << std::endl
		   << "#include \"interaction.hpp\"" << std::endl
		   << "#include <stdexcept>" << std::endl
		   << "#include <iostream>" << std::endl
		   << "#include <set>" << std::endl
		   << "#include \"agent_data_access.hpp\"" << std::endl
		   << "#define " << TAG_CRITICAL << "\n";
	
	// The syntax shortcuts for agents' info and other constants
	stream << "#define id_ 0\n"
		   << std::endl;

	// Add the prototypes of the Interaction types
	for (const auto &interaction : model.GetInteractions()) {
		stream << "class " << interaction.first << ";" << std::endl;
	}
	stream << std::endl;

	// Add the new class Agent
	stream << "class Agent {" << std::endl
		   << "public:" << std::endl
		   << "\tvirtual void Behavior()=0;" << std::endl;
	// Add the vector of received interactions (one for each Interaction type)
	stream << "protected:" << std::endl;
	for (const auto &interaction : model.GetInteractions()) {
		stream << "\tconst std::vector<" << interaction.first
			   << "> received_" << interaction.first
			   << ";" << std::endl;
	}
	
	// Add the function Send
	stream << std::endl
		   << "\tvoid Send(const Agent &destination, Interaction interaction);\n"
		   << "\tuint64_t TimeStep();" << std::endl
		   << "\tbool DoesAgentExist(uint64_t id, uint64_t type);\n"
		   << "\tuint64_t AgentIdTypeBound(uint64_t type);\n"
		   << "\tconst std::set<uint64_t> &GetAgentsOfType(uint64_t type) {std::set<uint64_t> *set = new std::set<uint64_t>(); return *set;}\n";

	stream << "};" << std::endl;
	stream << "#endif";

	return stream.str();
}


std::string GenerateInteractionHeaderContent() {
    std::stringstream stream;
    stream << "#ifndef INTERACTION_HPP_" << std::endl
		   << "#define INTERACTION_HPP_" << std::endl
		   << "#include <inttypes.h>\n"
		   << "#include \"consts.hpp\"\n"
		   << std::endl
		   << "class Interaction {\n"
		   << "protected:\n"
		   << "\tuint64_t sender_id_;\n"
		   << "\tuint64_t sender_type_;\n"
		   << "public:\n"
		   << "\tuint64_t GetSenderId() const {return sender_id_;}\n"
		   << "\tuint64_t GetSenderType() const {return sender_type_;}\n"
		   << "};" << std::endl
		   << std::endl
		   << "#endif";

    return stream.str();
}

std::string GenerateConstsHeaderContent(Model &model) {
	std::stringstream stream;
	stream << "#ifndef CONSTS_HPP_\n"
		   << "#define CONSTS_HPP_\n"
		   << "#include <inttypes.h>"
		   << "\n";
	
	for (const auto &agent : model.GetAgents()) 
		stream << "const uint64_t " << agent.first << TYPETAG << " = " << agent.second.GetId() << ";\n";
	
	for (const auto &interaction : model.GetInteractions()) 
		stream << "const uint64_t " << interaction.first << TYPETAG << " = " << interaction.second.GetId() << ";\n";
	
	stream << "\n#endif";
	
	return stream.str();
}

std::string GenerateAgentDataAccessStep1(Model &model) {
	std::stringstream stream;
	std::string main_file_name = model.GetSourceManager()->getFileEntryForID(
		model.GetSourceManager()->getMainFileID())->getName();
	ExtractMainDirectory(main_file_name);
	stream << "#ifndef AGENT_DATA_ACCESS_HPP_" << std::endl
		   << "#define AGENT_DATA_ACCESS_HPP_" << std::endl
		   << "#include <inttypes.h>\n"
		   << std::endl;

	stream << "template <class A>\n"
		   << "struct AgentContainer {\n"
		   << "\tconst A& operator[] (uint64_t s) {\n"
		   << "\t\tA* a;\n"
		   << "\t\treturn *a;\n"
		   << "\t}\n"
		   << "};\n";
	
	for (const auto &agent : model.GetAgents()) {
		stream << "class " << agent.first << ";\n";
		stream << "AgentContainer<" << agent.first << "> " << agent.first << "s;\n";
	}

	stream << "#endif\n";
	return stream.str();
}


void ConstructEnvironment(Model &model, clang::Rewriter &rewriter) {
	AddConstructorInInteraction(model, rewriter);
	AddBehaviorPrototypeInAgents(model, rewriter);
}
