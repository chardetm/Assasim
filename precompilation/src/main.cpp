#include <cstring>
#include <iostream>
#include <fstream>
#include <map>
#include <unordered_set>

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Basic/TargetInfo.h"

#include "utils.hpp"
#include "mpi_func.hpp"
#include "model.hpp"
#include "analyze_class.hpp"
#include "build_model.hpp"
#include "model_environment.hpp"

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory tool_category("my-tool options");

llvm::cl::opt<std::string> OutputToFolder("out-to-folder", llvm::cl::desc("Output edited and generated code in the specified folder"), llvm::cl::value_desc("folder"), llvm::cl::cat(tool_category));

llvm::cl::opt<bool> ToJson("to-json", llvm::cl::desc("Output the json corresponding to the model"), llvm::cl::cat(tool_category));

llvm::cl::opt<std::string> ToJsonFile("to-json-file", llvm::cl::desc("Export the json of the model in the file specified"), llvm::cl::value_desc("file"), llvm::cl::cat(tool_category));

llvm::cl::opt<bool> FirstStep("step1", llvm::cl::desc("First step of precompilation: generates the model coding environment for simplified C++ syntax."), llvm::cl::cat(tool_category));

llvm::cl::opt<bool> SecondStep("step2", llvm::cl::desc("Second step of precompilation: generates the final compilable code of the simulation."), llvm::cl::cat(tool_category));

llvm::cl::opt<std::string> ModelFileName("model-file", llvm::cl::desc("Gives the model file name for step2"), llvm::cl::cat(tool_category));

static llvm::cl::extrahelp common_help(clang::tooling::CommonOptionsParser::HelpMessage);

static llvm::cl::extrahelp more_help("More Help");

Model model;
clang::Rewriter rewriter;
std::unordered_set<PairLocation, hashPairLocation> CriticalLocation;
std::set<std::string> IncludedFiles;

int main(int argc, char **argv) {
	clang::tooling::CommonOptionsParser options_parser(argc, const_cast<const char**>(argv), tool_category);

	if (options_parser.getSourcePathList().size() != 1) {
		ErrorMessage() << "You must enter exactly one file to parse!";
		exit(-1);
	}

	clang::tooling::ClangTool tool(options_parser.getCompilations(),
	                               options_parser.getSourcePathList());

	std::unique_ptr<clang::tooling::FrontendActionFactory> build_model_factory = clang::tooling::newFrontendActionFactory<BuildModelFrontendAction>();
	tool.run(build_model_factory.get());

	// Free MPI database
	MPITypeMap::Free();
	return 0;
}
