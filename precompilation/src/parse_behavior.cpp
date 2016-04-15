#include "utils.hpp"
#include "parse_behavior.hpp"
#include "analyze_class.hpp"
#include "clang/Basic/OperatorKinds.h"

bool BehaviorVisitor::TraverseCXXMethodDecl(clang::CXXMethodDecl *decl) {
	method_name_ = decl->getNameAsString();
	TraverseStmt(decl->getBody());
	
	return true;
}

bool BehaviorVisitor::TraverseMemberExpr(clang::MemberExpr *expr) {
	clang::QualType base_type = expr->getBase()->getType().getUnqualifiedType();
	std::string base_name = base_type.getAsString();
	std::string member_name = expr->getMemberDecl()->getNameAsString();
	
	// Verify if the type is a class
	if (!base_type.getTypePtr()->isClassType()) {
		TraverseStmt(expr->getBase());
		return true;
	}
	
	base_name = base_name.substr(6); // Retrieve the actual name of the class
	
	// If the class is not an agent, we have nothing to do
	if (!model_.GetAgents().count(base_name)) {
		TraverseStmt(expr->getBase());
		return true;
	}
	
	AgentTypeContainer &agent(model_.GetAgents()[base_name]);
	
	if (auto op_expr = clang::dyn_cast<clang::CXXOperatorCallExpr>(expr->getBase())) {
		if (!agent.GetFields().count(member_name)) // If the field is invalid, do nothing
			return true;
		if (visited_member_expr_.count(expr->getLocStart())>0)
			return true;
		FieldTypeContainer &field = agent.GetFields()[member_name];
		
		expr_string_ = std::string();
		
		visit_operator_ = true;
		
		std::stringstream stream;
		
		stream << "(*((" << GetTypeAsString(field.GetType()) << "*)AskAttribute(" 
			   << field.GetId() << ",";
		
		rewriter_.ReplaceText(expr->getLocStart(), base_name.length()+2, stream.str());
		VisitCXXOperatorCallExpr(op_expr);
		
		stream.str("");
		stream << "," << model_.GetAgents()[base_name].GetId() << ")))";
		
		rewriter_.ReplaceText(expr->getLocEnd().getLocWithOffset(-2), member_name.length() + 2, stream.str());
		visited_member_expr_.insert(expr->getLocStart());
				
	}
	
	return true;
}

bool BehaviorVisitor::VisitCXXOperatorCallExpr(clang::CXXOperatorCallExpr *expr) {
	if (!visit_operator_)
		return true;
	
	visit_operator_ = false;
	std::string s;
	llvm::raw_string_ostream stream(s);
	std::string sn;
	llvm::raw_string_ostream name(sn);
	
	if (expr->getArg(1) == nullptr || std::string(clang::getOperatorSpelling(expr->getOperator())) != "[]")
		if (expected_operator_) {
			clang::FullSourceLoc loc = clang::FullSourceLoc(expr->getExprLoc(),*model_.GetSourceManager());
			ErrorMessage(loc) << "in method " << method_name_ << "of agent " << agent_name_ << ", invalid syntax for the recipient of Send.";
			model_.AddErrorFound();
			expected_operator_ = false;
			return true;
		}
	expected_operator_ = false;
	expr->getArg(0)->printPretty(name, nullptr, policy_);
	if (model_.GetAgents().count(name.str().substr(0,name.str().length()-1))==0)
		return true;
	
	TraverseStmt(expr->getArg(1));
	
	expr->printPretty(stream, nullptr, policy_);
	expr_string_ = stream.str();
	
	return true;
}

bool BehaviorVisitor::VisitCXXMemberCallExpr(clang::CXXMemberCallExpr *expr) {
	clang::QualType base_type = expr->getCallee()->getType().getUnqualifiedType();
	std::string base_name = base_type.getAsString();
	
	if (auto callee = clang::dyn_cast<clang::MemberExpr>(expr->getCallee())) {
		std::string s;
		llvm::raw_string_ostream implicit(s);
		
		callee->getBase()->printPretty(implicit, nullptr, policy_);
		if (implicit.str() != "this" || callee->getMemberDecl()->getNameAsString() != "Send") { // Treat only if it corresponds to a call to this->Send
			return true;
		}
		if (expr->getNumArgs() != 2)
			return true;
		// First parse the recipient information
		if (auto recipient_info = clang::dyn_cast<clang::CXXOperatorCallExpr>(expr->getArg(0)->IgnoreImpCasts())) {
			visit_operator_ = true;
			expected_operator_ = true;
			VisitCXXOperatorCallExpr(recipient_info);
			
			unsigned k = 0;
			while (k < expr_string_.length() && expr_string_[k] != '[')
				k++;
			if (!model_.GetAgents().count(expr_string_.substr(0,k-1))) {
				clang::FullSourceLoc loc = clang::FullSourceLoc(expr->getExprLoc(), *model_.GetSourceManager());
				ErrorMessage(loc) << "in method " << method_name_ << "of agent " << agent_name_ << ", invalid type of agent for the recipient in call of Send.";
				model_.AddErrorFound();
				return true;
			}
			
			const AgentTypeContainer &agent = model_.GetAgents()[expr_string_.substr(0,k-1)];
			// Then parse the interaction to send information
			if (auto interaction = clang::dyn_cast<clang::CXXConstructExpr>(expr->getArg(1)->IgnoreImpCasts())) {
				// Rewrite the beginning of the send function
				
				clang::Expr *c = nullptr;
				for (auto *child : interaction->children())
					c = clang::dyn_cast<clang::Expr>(child);
				
				if (auto inter_info = clang::dyn_cast<clang::CXXConstructExpr>(c->IgnoreCasts())) {
					std::string inter_name = inter_info->getConstructor()->getNameAsString();
					if (!model_.GetInteractions().count(inter_name)) {
						clang::FullSourceLoc loc = clang::FullSourceLoc(inter_info->getExprLoc(), *model_.GetSourceManager());
						ErrorMessage(loc) << "usage of Send in method " << method_name_ << "of agent " << agent_name_ << ": invalid interaction type.";
						model_.AddErrorFound();
					}
					const InteractionTypeContainer &interaction = model_.GetInteractions()[inter_name];
					std::stringstream stream;
					stream << "std::unique_ptr<Interaction>(new " << inter_name << "(" << interaction.GetId() << ",id_," << model_.GetAgents()[agent_name_].GetId() << ",";
					rewriter_.InsertText(expr->getLocStart().getLocWithOffset(4),"Message"); 
					rewriter_.ReplaceText(recipient_info->getLocStart(), k+1, stream.str());
					stream.str("");
					stream << "," << agent.GetId() << ",";
					for (unsigned i = 0; i<inter_info->getNumArgs(); i++) {
						std::string s;
						llvm::raw_string_ostream arg(s);
						
						inter_info->getArg(i)->printPretty(arg, nullptr, policy_);
						stream << arg.str() << ",";
					}
					stream.seekp(-1,std::ios_base::cur);
					stream << ")))";
					
					
					rewriter_.ReplaceText(clang::SourceRange(recipient_info->getLocEnd(), expr->getLocEnd()),stream.str());
					
				}
				else {
					clang::FullSourceLoc loc = clang::FullSourceLoc(expr->getExprLoc(), *model_.GetSourceManager());
					ErrorMessage(loc) << "usage of Send in method " << method_name_ << "of agent " << agent_name_ << ": syntax error in interaction to sent.";
					model_.AddErrorFound();
					return true;
				}
							
			}
			else {
				clang::FullSourceLoc loc = clang::FullSourceLoc(expr->getExprLoc(), *model_.GetSourceManager());
				ErrorMessage(loc) << "usage of Send in method " << method_name_ << "of agent " << agent_name_ << ": interaction to sent must be defined using a constructor.";
				model_.AddErrorFound();
				return true;
			}
		}
		else {
			clang::FullSourceLoc loc = clang::FullSourceLoc(expr->getExprLoc(), *model_.GetSourceManager());
			ErrorMessage(loc) << "usage of Send in method " << method_name_ << "of agent " << agent_name_ << ": invalid syntax for the recipient.";
			model_.AddErrorFound();
			return true;
		}
	}

	
	return true;
}
