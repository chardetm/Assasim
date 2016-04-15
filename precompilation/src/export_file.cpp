#define BOOST_NO_CXX11_SCOPED_ENUMS

#include <boost/filesystem.hpp>
#include <cstring>
#include <sstream>
#include <map>
#include <fstream>
#include <set>

#include "analyze_class.hpp"
#include "error_detection.hpp"
#include "utils.hpp"
#include "model.hpp"
#include "master_initialization.hpp"
#include "model_environment.hpp"
#include "generate_compilable_code.hpp"

void ExportGeneratedFilesStep1(std::string output_folder) {
    std::ofstream ofs;
    boost::filesystem::path dir(output_folder);
    boost::filesystem::create_directory(dir);
    BuildFolders(output_folder);

    ofs.open(output_folder+"/agent.hpp", std::ios::out);
    ofs << GenerateAgentHeaderContent(model);
    ofs.close();

	ofs.open(output_folder+"/agent_data_access.hpp", std::ios::out);
    ofs << GenerateAgentDataAccessStep1(model);
    ofs.close();

    ofs.open(output_folder+"/interaction.hpp", std::ios::out);
    ofs << GenerateInteractionHeaderContent();
    ofs.close();

	ofs.open(output_folder+"/consts.hpp", std::ios::out);
    ofs << GenerateConstsHeaderContent(model);
    ofs.close();

	ofs.open(output_folder+"/behaviors.cpp", std::ios::out);
    ofs << GenerateBehaviorsContent(model, rewriter);
    ofs.close();
}


std::string ExportModifiedFilesStep1(std::set<std::string> &IncludedFiles, std::string output_folder,
    std::string local_working, clang::Rewriter &rewriter, std::string automaticentry) {

    std::ofstream ofs;
    for (clang::Rewriter::const_buffer_iterator file_buffer = rewriter.buffer_begin();
        file_buffer != rewriter.buffer_end(); file_buffer++) {

        std::string file_name = rewriter.getSourceMgr().getFileEntryForID(
            file_buffer->first)->getName();

        IncludedFiles.erase(file_name);

        std::string end_file_name = file_name.substr(local_working.size(),
            file_name.size()-local_working.size());

        std::string true_file_name = output_folder + "/" + end_file_name;

        std::string directory = ExtractMainDirectory(true_file_name);
        BuildFolders(directory);
        true_file_name = directory + true_file_name;

        std::ifstream ifs;
        ifs.open(true_file_name);
        std::string entry;

        if (ifs.good()) {
            if (automaticentry != "ay") {
                WarningMessage() << "Warning: File" << true_file_name
                                << " already exist";
                do {
                    std::cerr << "Do you wish to overwrite it?\n"
                                << "yes for all: ay, yes: y, no: n\n";
                    std::cin >> entry;
                } while (entry != "y" && entry != "n" && entry != "ay");
            } else {
                WarningMessage() << "Warning: File" << true_file_name
                                << " was overwritten";
            }
        }
        if (entry == "ay") {
            automaticentry = "ay";
        }
        if (entry == "") {
            ofs.open(true_file_name , std::ios::out);
            if (ofs.is_open()) {
                ofs << std::string(file_buffer->second.begin(),
                    file_buffer->second.end());
            } else {
                ErrorMessage() << "No idea what can go wrong "
                                << "here but you can never have enough overkill";
            }
            ofs.close();
        } else {
            if (automaticentry == "ay" || entry == "y") {
                ofs.open(true_file_name , std::ios::trunc);
                if (ofs.is_open()) {
                    ofs << std::string(file_buffer->second.begin(),
                        file_buffer->second.end());
                } else {
                    ErrorMessage() << "No idea what can go wrong"
                                    << "here but you can never have enough overkill";
                }
                ofs.close();
            } else {
                WarningMessage() << "compilation result is not safe due to overwriting issues";
            }
        }
    }
    if (automaticentry == "ay") {
        return automaticentry;
    }
    return "";
}


void ExportNonModifiedFiles(std::set<std::string> &IncludedFiles, std::string local_working,
    std::string output_folder, std::string automaticentry) {
    // Create folders for copying unchanged files preserving the arborescence
    for (auto file_name: IncludedFiles) {
        std::string end_file_name = file_name.substr(local_working.size(),
            file_name.size()-local_working.size());

        std::string true_file_name = output_folder + "/" + end_file_name;
        std::string directory = ExtractMainDirectory(true_file_name);

        BuildFolders(directory);
        true_file_name = directory + true_file_name;

        if (boost::filesystem::exists(true_file_name)) {
            std::string entry = "";
            if (automaticentry != "ay") {
                WarningMessage() << "a non modified File " << true_file_name
                                << " already exists";
                do {
                        std::cerr << "Do you wish to overwrite it?\n"
                                << "yes for all: ay, yes: y, no: n\n";
                        std::cin >> entry;
                } while (entry != "y" && entry != "n" && entry != "ay");
            }
            if (entry == "ay") {
                automaticentry = "ay";
            }
            if (automaticentry == "ay") {
                boost::filesystem::remove(true_file_name);
                boost::filesystem::copy_file(file_name,true_file_name);
                if (entry != "ay") {
                    WarningMessage() << "File: " << true_file_name << " was overwritten";
                }
            } else {
                if (entry == "y") {
                    boost::filesystem::remove(true_file_name);
                    boost::filesystem::copy_file(file_name,true_file_name);
                } else {
                    ErrorMessage() << "compilation aborted due to overwriting issues";
                    exit(-1);
                }
            }
        } else {
            boost::filesystem::copy_file(file_name,true_file_name);
        }
    }
}

void ExportFixedFilesStep2(std::string output_folder) {
    boost::filesystem::path working;
    working = boost::filesystem::current_path();
    std::string working_directory = working.string();

    std::string src = ExtractMainDirectory(working_directory);
    src = src + "precompilation/simulation_basis/";
    std::string src_utils = src + "utils";
    std::string src_libs = src + "libs";

    std::string utils = output_folder + "/utils/";
    std::string libs = output_folder + "/libs/";

    BuildFolders(output_folder);
    BuildFolders(utils);
    BuildFolders(libs);

    // copy and paste of the files in simulation_basis
    for (boost::filesystem::directory_iterator file(src);
    file != boost::filesystem::directory_iterator(); ++file) {

        boost::filesystem::path current(file->path());
        std::string file_name = current.string();
        ExtractMainDirectory(file_name);

        if (file_name == "README" || file_name == "CMakeLists.txt"
        || file_name.substr(file_name.size()-2, file_name.size()) == "pp") {
            std::string true_file_name = output_folder + "/" + file_name;

            boost::filesystem::remove(true_file_name);
            boost::filesystem::copy_file(current, true_file_name);
        }
    }
    // copy and paste of the files in simulation_basis/utils
    CopyFiles(src_utils, utils);
    // copy and paste of everything in simulation_basis/libs
    CopyFiles(src_libs, libs);
}

void ExportGeneratedFilesStep2(std::string output_folder, clang::ASTContext *context_) {
    std::ofstream ofs;
    boost::filesystem::path dir(output_folder);
    boost::filesystem::create_directory(dir);
    BuildFolders(output_folder);

	ofs.open(output_folder+"/simulation_structs.hpp", std::ios::out);
	ofs << GenerateStructFile(model);
	ofs.close();
    ofs.open(output_folder+"/parameters_generation.cpp", std::ios::out);
    ofs << GenerateMasterInitialization(model, context_);
    ofs.close();
	ofs.open(output_folder+"/agent_model.cpp",std::ios::out);
	ofs << GenerateAgentCpp(model);
	ofs.close();
	ofs.open(output_folder+"/user_interface_model.cpp", std::ios::out);
	ofs << GenerateUserInterfaceModelCpp(model);
	ofs.close();

	model.WriteEmptyInstance(output_folder+"/empty_instance.json");
}
