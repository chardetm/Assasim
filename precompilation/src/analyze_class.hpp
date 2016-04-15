/**
 * \file analyze_class.hpp
 * \brief Provides the prototypes of functions which analyze classes and structures.
 */

#ifndef ANALYZE_CLASS_HPP_
#define ANALYZE_CLASS_HPP_

#include <cstring>
#include <map>

#include "clang/AST/ASTContext.h"
#include "clang/AST/Type.h"
#include "clang/AST/DeclCXX.h"
#include "llvm/Support/raw_ostream.h"

/**
 * Returns the declaration corresponding to the record type
 */
clang::CXXRecordDecl *GetDeclarationOfClass(const clang::QualType &type);

/**
 * Outputs true iff the class defined in type is a descendent of class parent
 */
bool InheritsFrom(clang::QualType &type, clang::ASTContext *context, const char *parent);
bool InheritsFrom(clang::CXXRecordDecl *declaration, clang::ASTContext* context, const char* parent);

/**
 * Outputs the type as a string. If type is a structure, recursively print
 * the fields of the structure.
 */
std::string GetTypeAsString(const clang::QualType &type);

/**
 * Outputs true iff the type is just combinations of struct and integral types.
 * Particularly, structures using pointers are not allowed.
 * Example: on input struct{ struct{int a; Interaction b} c; double d;}, it outputs
 * false since Interaction is a class, but it returns true on input
 * struct{ struct{int a; char b} c; double d;}.
 */
bool IsStructuralType(const clang::QualType &type, const clang::ASTContext *context);

/**
   Outputs true if the given method if of the good type i.e. of the form
   /code void Behavior() /endcode
 */
bool IsTrueBehavior(const clang::CXXMethodDecl *decl);

#endif
