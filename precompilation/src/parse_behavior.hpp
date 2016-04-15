/**
   \file parse_behavior.hpp
   \brief Step 2: parsing the code defining behaviors and replacing simplified syntax with the actual syntax.
   
   \todo Decide what to do if Send is used without direct call to a constructor of the interaction.
   \todo See how to provide the user with some constants of the simulation, like time step or maximum id of an Agent for each agent type.
   \todo Syntax for giving birth to new agents?
 */

#ifndef PARSE_BEHAVIOR_HPP_
#define PARSE_BEHAVIOR_HPP_

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>

#include "utils.hpp"
#include "model.hpp"

class BehaviorVisitor : public clang::RecursiveASTVisitor<BehaviorVisitor> {
public:
	explicit BehaviorVisitor(clang::ASTContext *context_p_, Model &model_p_, clang::Rewriter &rewriter_p_, std::string agent_name_p_) :
		context_(context_p_), model_(model_p_), rewriter_(rewriter_p_), agent_name_(agent_name_p_), visit_operator_(false), expected_operator_(false),lang_options_(), policy_(lang_options_) {
	}

	/// Traverse recursively all methods in Agents
	bool TraverseCXXMethodDecl(clang::CXXMethodDecl *decl);
	
	/// Visit any access to a member of a class. Only parse it if it is corresponds
	/// to an access to an attribute of an Agent. If it does, replace recursively
	/// all the accesses.
	/// It can handle nested public attribute requests of the form
	bool TraverseMemberExpr(clang::MemberExpr *expr);
	
	/// Visit an operator call and check if it corresponds to the use of operator [].
	/// If it is the case, retrieve the code between the brackets. 
	bool VisitCXXOperatorCallExpr(clang::CXXOperatorCallExpr *expr);
	
	/// Visit a call to the Send method of class Agent
	bool VisitCXXMemberCallExpr(clang::CXXMemberCallExpr *expr);

	/// Retrieve the actual constructor type for an interaction
	bool TraverseCXXContructExpr(clang::CXXConstructExpr *expr);
	
private:
	clang::ASTContext *context_;
	Model &model_;
	clang::Rewriter &rewriter_;
	std::string agent_name_;
	bool visit_operator_;
	bool expected_operator_;
	std::string expr_string_;
	std::string method_name_;
	std::unordered_set<clang::SourceLocation, hashSourceLocation> visited_member_expr_;
	
	/* Internal libclang objects for printing expressions */
	clang::LangOptions lang_options_;
	clang::PrintingPolicy policy_;
};

#endif
