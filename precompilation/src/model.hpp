/**
 * \file model.hpp
 * \brief Defines structures helping to describe a model.
 */

#ifndef MODEL_HPP_
#define MODEL_HPP_

#include <cstring>
#include <sstream>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <fstream>

#include "clang/Rewrite/Core/Rewriter.h"

#include "analyze_class.hpp"
#include "utils.hpp"
#include "mpi_func.hpp"

/**
 * \class FieldTypeContainer
 * \brief Contains relevant information on a field (attribute) of a class.
 *
 * A FieldTypeContainer contains the type of the attribute, its id, its
 * access mode (public, private, protected) and its corresponding MPI_Datatype.
 */
class FieldTypeContainer {
public:
	FieldTypeContainer() : type_(), id_(0), access_(clang::AS_none), is_sendable_(true) {}

	FieldTypeContainer(const clang::QualType &type_p_, int64_t id_p_, clang::AccessSpecifier access_p_, bool is_critical_p_ = false) :
		type_(type_p_), id_(id_p_), access_(access_p_), is_critical_(is_critical_p_), is_sendable_(true) {}

	const clang::QualType &GetType() const {
		return type_;
	}

	const int64_t &GetId() const {
		return id_;
	}

	const clang::AccessSpecifier &GetAccess() const {
		return access_;
	}

	bool IsCritical() const {
		return is_critical_;
	}

	bool IsSendable() const {
		return is_sendable_;
	}

	void SetNotSendable() {
		is_sendable_ = false;
	}

private:
	clang::QualType type_;
	int64_t id_;
	clang::AccessSpecifier access_;
	bool is_critical_;
	bool is_sendable_;
};

// Stores the fields of a class by name
typedef std::unordered_map<std::string,FieldTypeContainer> FieldMemory;


/* Storing classes */

/**
 * \class ClassTypeContainer
 * \brief Contains relevant information on a class.
 *
 * A ClassTypeContainer of a class contains its type, its id, all its fields and
 * the file where it is defined.
 * In this program, it is used for classes that inherit either Agent or
 * Interaction.
 */
class ClassTypeContainer {
public:
	ClassTypeContainer() : type_(), id_(0), index_fields_(0) {}

	// Little constructor dedicated to direct function usage
	ClassTypeContainer(const clang::QualType &type_p_) : type_(type_p_) {}

	/**
	 * Adds the fields of the class defined by type (and in its bases recursively)
	 * in the field memory
	 */
	void AddFields(const clang::QualType &type);

	ClassTypeContainer(const clang::QualType &type_p_, int64_t id_p_, clang::FileID file_p_) :
		type_(type_p_), id_(id_p_), index_fields_(0), file_(file_p_) {
		// Store fields' info
		AddFields(type_);
	}

	const clang::QualType &GetType() const {
		return type_;
	}

	const int64_t &GetId() const {
		return id_;
	}

	const clang::FileID &GetFile() const {
		return file_;
	}

	/// Returns the declaration associated to the class
	const clang::CXXRecordDecl *GetDecl() const {
		return GetDeclarationOfClass(type_);
	}

	FieldMemory &GetFields() {
		return fields_;
	}

	const FieldMemory &GetFields() const {
		return fields_;
	}

protected:
	clang::QualType type_; /// Type defining the class
	FieldMemory fields_;
	int64_t id_;
	int64_t index_fields_;
	clang::FileID file_; /// File containing the class
};

class AgentTypeContainer : public ClassTypeContainer {
public:
	AgentTypeContainer() : ClassTypeContainer(), is_sendable_(true) {}

	AgentTypeContainer(const clang::QualType &type_p_, int64_t id_p_, clang::FileID file_p_) : ClassTypeContainer(type_p_, id_p_, file_p_), is_sendable_(true) {}

	/**
	 * Checks if the class has a Behavior method (mandatory for agents).
	 */
	bool HasBehavior() const;

	bool IsSendable() const {
		return is_sendable_;
	}

	void SetNotSendable() {
		is_sendable_ = false;
	}

	/**
	 * Returns the code defining the struct of all attributes of the agent
	 */
	std::string AttributesStruct(const std::string &name) const;

	/**
	 * Returns the code defining the struct of all public attributes of the agent
	 */
	std::string PublicAttributesStruct(const std::string &name) const;

	/**
	 * Returns the code defining the struct of all critical attributes of the agent
	 */
	std::string CriticalAttributesStruct(const std::string &name) const;

	/**
	 * Returns the code defining the actual struct used for sending an agent
	 */
	std::string MessageStruct(const std::string &name) const;

private:
	bool is_sendable_;
};

class InteractionTypeContainer : public ClassTypeContainer {
public:
	InteractionTypeContainer() : ClassTypeContainer() {}

	InteractionTypeContainer(const clang::QualType &type_p_, int64_t id_p_, clang::FileID file_p_) : ClassTypeContainer(type_p_, id_p_, file_p_) {}

	/**
	 * Checks if the class already contains a complete constructor
	 */
	bool HasCompleteConstructor() const;

	/**
	 * Returns the code defining the struct of all attributes (which should be public)
	 * of the interaction
	 */
	std::string AttributesStruct(const std::string &name) const;

	/**
	 * Returns the code defining the actual struct used for sending an interaction
	 */
	std::string MessageStruct(const std::string &name) const;
};

/// Store Agent type classes indexed by their name
typedef std::unordered_map<std::string,AgentTypeContainer> AgentTypeMemory;

/// Store Interaction type classes indexed by their name
typedef std::unordered_map<std::string,InteractionTypeContainer> InteractionTypeMemory;

/* Storing models */

/**
 * \class Model
 * \brief Contains relevant information on a model.
 *
 * A Model of a model contains all types of agents and all types of interactions
 * defined in the model.
 *
 * \todo Implement ExportModel.
 */
class Model {
public:
	Model() : index_agents_(0), index_interactions_(0), error_counter_(0), warning_counter_(0), source_manager_(NULL) {}

	Model(clang::SourceManager *source_manager_p_, std::string model_file_name_p_) : index_agents_(0), index_interactions_(0), error_counter_(0), warning_counter_(0), source_manager_(source_manager_p_), model_file_name_(model_file_name_p_) {}

	/**
	 * Adds an agent to the model.
	 */
	void AddAgent(std::string name, clang::QualType type, clang::FileID file) {
		// First verify that the class is not abstract
		if (!GetDeclarationOfClass(type)->isAbstract()) {
			agents_[name] = AgentTypeContainer(type, index_agents_, file);
			index_agents_++;
		}
		else {
			clang::FullSourceLoc loc = clang::FullSourceLoc(GetDeclarationOfClass(type)->getLocStart(), *source_manager_);
			WarningMessage(loc) << "class " << name << " is abstract, it cannot be used as an Agent";
			AddWarningFound();
		}
	}

	/**
	 * Adds an interaction to the model.
	 */
	void AddInteraction(std::string name, clang::QualType type, clang::FileID file) {
		interactions_[name] = InteractionTypeContainer(type, index_interactions_, file);
		index_interactions_++;
	}

	const AgentTypeMemory &GetAgents() const {
		return agents_;
	}

	AgentTypeMemory &GetAgents() {
		return agents_;
	}

	const InteractionTypeMemory &GetInteractions() const {
		return interactions_;
	}

	InteractionTypeMemory &GetInteractions() {
		return interactions_;
	}

	void AddErrorFound() {
		error_counter_++;
	}

	void AddWarningFound() {
		warning_counter_++;
	}

	/**
	 * Computes the MPI_Datatypes of every public attributes of agents and
	 * interactions.
	 */
	void ComputeMPIDatatypes(clang::ASTContext *context);

	/**
	 * Returns the binary JSON description of the model.
	 */
	std::string ExportModel();

	const unsigned &GetErrorCounter() const {
		return error_counter_;
	}

	const unsigned &GetWarningCounter() const {
		return warning_counter_;
	}

	const clang::SourceManager *GetSourceManager() const {
		return source_manager_;
	}
	
	const std::string &GetModelFileName() const {
		return model_file_name_;
	}
	
	bool WriteBinaryJson(const std::string &file) const;
	std::ostream& PrintJson(std::ostream &ost, bool to_string=false) const;
	void WriteJson(const std::string &file, bool to_string=false) const {
		std::ofstream f(file);
		PrintJson(f, to_string);
	}
	
	std::ostream& PrintEmptyInstance(std::ostream &ost) const;
	void WriteEmptyInstance(const std::string &file) const {
		std::ofstream f(file);
		PrintEmptyInstance(f);
	}

private:
	AgentTypeMemory agents_;
	int64_t index_agents_;

	InteractionTypeMemory interactions_;
	int64_t index_interactions_;

	/// Counts the number of errors found
	unsigned error_counter_;
	/// Counts the number of warnings
	unsigned warning_counter_;

	/// SourceManager used to print information about locations
	clang::SourceManager *source_manager_; 
	std::string model_file_name_;
};

#endif
