/**
 * \file build_model.hpp
 * \brief Defines classes aiming for analysing the input model and extract its
 *        content.
 */

#ifndef BUILD_MODEL_HPP_
#define BUILD_MODEL_HPP_

#include <cstring>
#include <map>
#include <unordered_set>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Lex/Preprocessor.h>

#include "utils.hpp"
#include "analyze_class.hpp"
#include "model.hpp"
#include "model_environment.hpp"

extern Model model;
extern clang::Rewriter rewriter;
extern std::unordered_set<PairLocation, hashPairLocation> CriticalLocation;

/**
 * \class BuildModelVisitor
 * \brief Visitor which explores the AST and stores the relevant info in its model.
 */
class BuildModelVisitor
	: public clang::RecursiveASTVisitor<BuildModelVisitor> {
public:
    explicit BuildModelVisitor(clang::ASTContext *context_p_, Model &model_p_,
	clang::DiagnosticsEngine &diag_engine_p_)
		: context_(context_p_), model_(model_p_), diag_engine_(diag_engine_p_) {}

	/**
	 * For each class definition, stores the informations about the class if it
	 * is an agent or an interaction.
	 */
    bool VisitCXXRecordDecl(clang::CXXRecordDecl *declaration);

private:
	clang::ASTContext *context_;
	/// Contains the info to be kept after the parsing operation
	Model &model_;
	clang::DiagnosticsEngine &diag_engine_;
};

/**
 * \class BuildModelConsumer
 * \brief Custom ASTConsumer containing the custom recursive visitor
 */
class BuildModelConsumer : public clang::ASTConsumer {
public:
	explicit BuildModelConsumer(clang::ASTContext *context, Model &model,
	clang::DiagnosticsEngine &diag_engine)
		: visitor_(context, model, diag_engine) {}

	virtual void HandleTranslationUnit(clang::ASTContext &context);

private:
	BuildModelVisitor visitor_;
};

/**
 * \class FindCriticalUse
 * \brief Class used to determine when the keyword critical is used.
 */
class FindCriticalUse : public clang::PPCallbacks {
	//Fix -Woverloaded-virtual
	using clang::PPCallbacks::MacroExpands;
public:
	FindCriticalUse(clang::SourceManager &source_manager_p_) : source_manager_(source_manager_p_) {}

	/// Method called each time a macro invocation is encountered
	void MacroExpands(const clang::Token &macro_token, const clang::MacroDirective *macro_directive,
		 clang::SourceRange source_range, const clang::MacroArgs *args);

private:
	clang::SourceManager &source_manager_;
};

/**
 * \class FindIncluded
 * \brief Class used to recursively find all files included in the main file.
 */
class FindIncluded : public clang::PPCallbacks {
public:
	FindIncluded(clang::SourceManager &source_manager_p_) : source_manager_(source_manager_p_) {}

	void InclusionDirective(clang::SourceLocation HashLoc, const clang::Token & IncludeTok,
		llvm::StringRef FileName, bool IsAngled, clang::CharSourceRange FilenameRange,
		const clang::FileEntry *File, llvm::StringRef SearchPath, llvm::StringRef RelativePath,
		const clang::Module *Imported);

private:
	clang::SourceManager &source_manager_;
};

/**
 * \class BuildModelFrontendAction
 * \brief FrontendAction to initiate the parsing.
 */
class BuildModelFrontendAction : public clang::ASTFrontendAction {
public:
	BuildModelFrontendAction() : model_(model) {}

	/// Initialization before parsing main file
	std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &CI,
		clang::StringRef file);

	/// Actions to perform after the file is parsed
	void EndSourceFileAction();

private:
	Model &model_;
	clang::ASTContext *context_;
};

#endif
