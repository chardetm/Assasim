/**
 * \file build_model.cpp
 * \brief Implements some methods of BuildModelVisitor and BuildModelConsumer.
 */

#include <cstring>
#include <sstream>
#include <map>
#include <fstream>
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>

#include "analyze_class.hpp"
#include "error_detection.hpp"
#include "build_model.hpp"
#include "utils.hpp"
#include "model.hpp"
#include "master_initialization.hpp"
#include "model_environment.hpp"
#include "export_file.hpp"
#include "generate_compilable_code.hpp"
#include "parse_behavior.hpp"

bool BuildModelVisitor::VisitCXXRecordDecl(clang::CXXRecordDecl *declaration) {
	// We are only interested in class definition having a body
	if (declaration->isClass() && declaration->isCompleteDefinition()) {
		clang::QualType type = context_->getTypeDeclType(declaration);
		std::string name = declaration->getQualifiedNameAsString();
		clang::FullSourceLoc loc = context_->getFullLoc(declaration->getLocStart());
		clang::FileID file = loc.getFileID();

		// Check whether this class defines an agent or an interaction
		bool is_interaction = InheritsFrom(declaration, context_, "Interaction");
		bool is_agent = InheritsFrom(declaration, context_, "Agent");

		if (is_agent && is_interaction) {

			ErrorMessage(loc) << name << " is defined as an interaction and an agent";
			model_.AddErrorFound();
			return true;
		}

		if (is_agent) {
			// Store the agent in the model
			model_.AddAgent(name, type, file);

		} else if (is_interaction) {
			// Store the interaction in the model
			model_.AddInteraction(name, type, file);
		}
	}
	// Proceed with the parsing of the AST
	return true;
}


void BuildModelConsumer::HandleTranslationUnit(clang::ASTContext &context) {
	visitor_.TraverseDecl(context.getTranslationUnitDecl());
}


void FindCriticalUse::MacroExpands(const clang::Token &macro_token,
	 const clang::MacroDirective *macro_directive, clang::SourceRange source_range,
	 const clang::MacroArgs *args) {
	// If the macro called is the one defined as TAG_CRITICAL, then store
	// its location in the vector CriticalLocation.
	if (macro_token.getIdentifierInfo()->getName() == TAG_CRITICAL) {
		clang::SourceLocation source_location = macro_token.getLocation();
		CriticalLocation.insert(PairLocation(source_manager_.getFileID(source_location),
			source_manager_.getSpellingLineNumber(source_location)));
	}
}


extern std::set<std::string> IncludedFiles;


void FindIncluded::InclusionDirective(clang::SourceLocation HashLoc, const clang::Token& IncludeTok,
	llvm::StringRef FileName, bool IsAngled, clang::CharSourceRange FilenameRange,
	const clang::FileEntry *File, llvm::StringRef SearchPath, llvm::StringRef RelativePath,
	const clang::Module *Imported) {

	std::string main_file_name = model.GetSourceManager()->getFileEntryForID(
		model.GetSourceManager()->getMainFileID())->getName();
	std::string working_folder = ExtractMainDirectory(main_file_name);

	if (IncludeTok.getIdentifierInfo()->getName() == "include") {
		if (SearchPath == working_folder.substr(0,working_folder.size()-1)) {
			IncludedFiles.insert(SearchPath.str()+"/"+FileName.str());
		}
	}
}

extern llvm::cl::opt<std::string> ModelFileName;
std::unique_ptr<clang::ASTConsumer> BuildModelFrontendAction::CreateASTConsumer(
	clang::CompilerInstance &CI, clang::StringRef file) {

	context_ = &(CI.getASTContext());
	rewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
	model = Model(&CI.getSourceManager(),ModelFileName);

	std::unique_ptr<FindCriticalUse> find_critical(new FindCriticalUse(CI.getSourceManager()));
	CI.getPreprocessor().addPPCallbacks(std::move(find_critical));


	std::unique_ptr<FindIncluded> find_included(new FindIncluded(CI.getSourceManager()));
	CI.getPreprocessor().addPPCallbacks(std::move(find_included));

	return llvm::make_unique<BuildModelConsumer>(&(CI.getASTContext()), model_, CI.getDiagnostics());
}


extern llvm::cl::opt<bool> FirstStep;
extern llvm::cl::opt<bool> SecondStep;
extern llvm::cl::opt<bool> ToJson;
extern llvm::cl::opt<std::string> ToJsonFile;

void BuildModelFrontendAction::EndSourceFileAction() {
	extern llvm::cl::opt<std::string> OutputToFolder;
	CheckErrorInModel(model, context_);

	if (model.GetWarningCounter()) {
		llvm::errs() << model.GetWarningCounter() << " warning";
		if (model.GetWarningCounter() > 1)
			llvm::errs() << "s";
		llvm::errs() << " raised during parsing.\n";
	}
	if (model.GetErrorCounter()) {
		llvm::errs() << "Compilation aborted: " << model.GetErrorCounter() << " error";
		if (model.GetErrorCounter() > 1)
			llvm::errs() << "s";
		llvm::errs() << " found.\n";
		exit(-1);
	} else {
		llvm::errs() << "Parsing successful\n";
		
		if (ToJson) {
			model.PrintJson(std::cout);
		}
		else if (ToJsonFile != "") {
			model.WriteBinaryJson(ToJsonFile);
		}
		else if (FirstStep) {
			ConstructEnvironment(model, rewriter);
			if (OutputToFolder == "") {
				llvm::outs() << "### File agent.hpp ###\n" << GenerateAgentHeaderContent(model)
							 << "\n######################\n";
				llvm::outs() << "### File agent_data_access.hpp ###\n"
							 << GenerateAgentDataAccessStep1(model)
							 << "##################################\n";
				for (clang::Rewriter::const_buffer_iterator file_buffer = rewriter.buffer_begin();
					file_buffer != rewriter.buffer_end(); file_buffer++) {

					llvm::outs() << "\n\n### File " << rewriter.getSourceMgr().getFileEntryForID(
						file_buffer->first)->getName() << " ###\n";

					llvm::outs() << std::string(file_buffer->second.begin(),
						file_buffer->second.end());
				}
			} else {
				//local_folder is the arborescence of the folder where everything will be add
				//during the copy-past and modification of the files
				std::string output_folder;

				//local_working is the arborescence of the working directory
				std::string local_working;
				std::string main_file_name;

				main_file_name = model.GetSourceManager()->getFileEntryForID(
					model.GetSourceManager()->getMainFileID())->getName();

				local_working = ExtractMainDirectory(main_file_name);

				IncludedFiles.erase(local_working + "agent.hpp");
				IncludedFiles.erase(local_working + "interaction.hpp");
				IncludedFiles.erase(local_working + "agent_data_access.hpp");

				output_folder = local_working + OutputToFolder;

				ExportGeneratedFilesStep1(output_folder);

				std::string automaticentry = "";

				automaticentry = ExportModifiedFilesStep1(IncludedFiles, output_folder,
														  local_working, rewriter, automaticentry);

				ExportNonModifiedFiles(IncludedFiles, local_working, output_folder,
					automaticentry);
			}
		} else if (SecondStep) {
			if (ModelFileName == "") {
				ErrorMessage() << "wrong options for step2, you need to specify the model file name";
				exit(EXIT_FAILURE);
			}
			for (const auto &agent : model.GetAgents()) {
				BehaviorVisitor visitor = BehaviorVisitor(context_, model, rewriter, agent.first);
				visitor.TraverseCXXRecordDecl(GetDeclarationOfClass(agent.second.GetType()));
			}
			AddConstructorsInInteractionsStep2(model, rewriter);
			AddReceivedInteractionsInAgents(model, rewriter);
			AddPrototypesInAgentsStep2(model, rewriter);
			if (OutputToFolder == "") {
				llvm::outs() << "### File simulation_structs.hpp ###\n"
							 << GenerateStructFile(model)
							 << "###################################\n";
				llvm::outs() << "### File parameters_generation.cpp ###\n"
							 << GenerateMasterInitialization(model, context_)
							 << "##################################\n";
				llvm::outs() << "### File agent_model.cpp ###\n"
							 << GenerateAgentCpp(model)
							 << "######################\n";
				llvm::outs() << "### File cli_model.cpp ###\n"
							 << GenerateUserInterfaceModelCpp(model)
							 << "##########################\n";
				for (clang::Rewriter::const_buffer_iterator file_buffer = rewriter.buffer_begin();
					file_buffer != rewriter.buffer_end(); file_buffer++) {

					llvm::outs() << "\n\n### File " << rewriter.getSourceMgr().getFileEntryForID(
						file_buffer->first)->getName() << " ###\n";

					llvm::outs() << std::string(file_buffer->second.begin(),
						file_buffer->second.end());
				}
			} else {
                //local_folder is the arborescence of the folder where everything will be add
				//during the copy-past and modification of the files
				std::string output_folder;

				//local_working is the arborescence of the working directory
				std::string local_working;
				std::string main_file_name;

				main_file_name = model.GetSourceManager()->getFileEntryForID(
					model.GetSourceManager()->getMainFileID())->getName();

				local_working = ExtractMainDirectory(main_file_name);
				
				
				IncludedFiles.insert(local_working+"behaviors.cpp");
				
				IncludedFiles.erase(local_working + "agent.hpp");
				IncludedFiles.erase(local_working + "interaction.hpp");
				IncludedFiles.erase(local_working + "agent_data_access.hpp");
								
				output_folder = local_working + OutputToFolder;
				
				ExportFixedFilesStep2(output_folder);
				ExportGeneratedFilesStep2(output_folder, context_);
				
				std::string automaticentry = "";
				
				automaticentry = ExportModifiedFilesStep1(IncludedFiles, output_folder, local_working, rewriter, automaticentry);
				
				ExportNonModifiedFiles(IncludedFiles, local_working, output_folder, automaticentry);
			}
		}
	}
}
