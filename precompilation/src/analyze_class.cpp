/**
 * \file analyze_class.cpp
 * \brief Provides the implementation of functions which analyze classes and
 *        structures.
 */

#include <cstring>
#include <map>
#include <sstream>

#include "analyze_class.hpp"
#include "model.hpp"

clang::CXXRecordDecl *GetDeclarationOfClass(const clang::QualType &type) {
	const clang::RecordType *temp = type.getTypePtr()->getAs<clang::RecordType>();
	if (temp == nullptr)
		return nullptr;
	return static_cast<clang::CXXRecordDecl*>(temp->getDecl());
}

bool InheritsFrom(clang::QualType &type, clang::ASTContext* context, const char* parent) {
	const clang::CXXRecordDecl *declaration = GetDeclarationOfClass(type);
	if (declaration == nullptr)
		return false;
	for (const auto& base : declaration->bases()) {
        // See all the written parents (bases) of the class
		clang::QualType base_type = context->getCanonicalType(base.getType());
		if (base_type.getBaseTypeIdentifier()->getName() == parent) // Ok if directly inherits
			return true;
		if (InheritsFrom(base_type, context, parent)) // Ok if inherits of a descendent
			return true;
	}
	return false;
}

bool InheritsFrom(clang::CXXRecordDecl *declaration, clang::ASTContext* context, const char* parent) {
	for (const auto& base : declaration->bases()) {
        // See all the written parents (bases) of the class
		clang::QualType base_type = context->getCanonicalType(base.getType());
		if (base_type.getBaseTypeIdentifier() != nullptr && base_type.getBaseTypeIdentifier()->getName() == parent) // Ok if directly inherits
			return true;
		if (InheritsFrom(base_type, context, parent)) // Ok if inherits of a descendent
			return true;
	}
	return false;
}


std::string GetTypeAsString(const clang::QualType &type) {
	std::stringstream stream;
	std::string name = type.getAsString();
    // If it is an anonymous structure, print all the fields recursively
    if (name.substr(0,11) == "struct (ano") {
        stream << "struct { ";
		clang::RecordDecl* struct_decl = type.getTypePtr()->getAsStructureType()->getDecl();
		// Print the types of all the fields
        for (const auto* field : struct_decl->fields()) {
			std::string type = GetTypeAsString(field->getType().getCanonicalType());
			std::string name = field->getName();
            stream << type;
            stream << " " << name << "; ";
        }
        stream << "}";
	} else if (type.getTypePtr()->isBooleanType()) {
		return "bool";
	} else {
        stream << type.getAsString(); //just print the type
    }
	return stream.str();
}

bool IsStructuralType(const clang::QualType& type, const clang::ASTContext *context) {
	// If it is struct, check if all fields are of structural type
	if (type.getCanonicalType().getTypePtr()->isStructureType()) {
		clang::RecordDecl* struct_decl = type.getCanonicalType().getTypePtr()->getAsStructureType()->getDecl();
		for (const auto* field : struct_decl->fields()) {
			if (!IsStructuralType(field->getType(),context))
				return false;
		}
		return true;
	} else { // Else check if it is of integral type
		return type.getCanonicalType().getTypePtr()->isBuiltinType();
	}
}

bool IsTrueBehavior(const clang::CXXMethodDecl *decl) {
	return ((decl->getName() == "Behavior") && (decl->param_size() == 0) && (decl->getReturnType().getTypePtr()->isVoidType()));
}
