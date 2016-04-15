/**
 * \file export_file.hpp
 * \brief Handles file export for the precompilation step.
 */

#ifndef EXPORT_FILE_HPP_
#define EXPORT_FILE_HPP_

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

/**
 * Creates Agent.hpp and Interaction.hpp in the output_folder.
 */
void ExportGeneratedFilesStep1(std::string output_folder);

/**
 * Creates every modified files modified during step1 in their respective folder in the output_folder.
 */
std::string ExportModifiedFilesStep1(std::set<std::string> &IncludedFiles, std::string output_folder,
    std::string local_working, clang::Rewriter &rewriter, std::string automaticentry);

/**
 * Copies every non modified files in their respective folder in the output_folder.
 */
void ExportNonModifiedFiles(std::set<std::string> &IncludedFiles, std::string local_working,
    std::string output_folder, std::string automaticentry);

/**
 * Copy agent.hpp, interaction.hpp, master.hpp in the output_folder. We suppose we are executing the
 * program from assasim/precompilation.
 */
void ExportFixedFilesStep2(std::string output_folder);

/**
 * Creates parameters_generation.cpp in the output_folder.
 */
void ExportGeneratedFilesStep2(std::string output_folder, clang::ASTContext *context_);

#endif
