/**
 * \file utils.hpp
 * \brief Handles the output of the analysis of the precompilation step.
 */

#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <string>
#include <iostream>

#include "clang/Basic/SourceLocation.h"

/// Tag used in critical attribute definition
#define TAG_CRITICAL "$critical"

// Cross-platform library for printing colors in console
#include "rlutil.hpp"

/* Specific messages */

/**
 * \class ErrorMessage
 * \brief Class used to print an error message in red color using stream
 * operators.
 *
 * Example:
 * \code{.cpp} ErrorMessage() << "Error at line" << 12 << endl; \endcode
 */
class ErrorMessage {
public:
	ErrorMessage(std::ostream &stream=std::cerr) : stream_(stream) {
		rlutil::setColor(rlutil::LIGHTRED);
		stream_ << "Error: ";
		rlutil::resetColor();
	}

	ErrorMessage(const clang::FullSourceLoc &loc, std::ostream &stream=std::cerr) : stream_(stream) {
		rlutil::setColor(rlutil::WHITE);
		stream_ << loc.printToString(loc.getManager()) << ": ";
		rlutil::setColor(rlutil::LIGHTRED);
		stream << "error: ";
		rlutil::resetColor();
	}

	~ErrorMessage() {
		rlutil::resetColor();
		stream_ << "\n\n";
	}

	template<class T>
	ErrorMessage &operator<< (const T &right) {
		stream_ << right;
		return *this;
	}

private:
	std::ostream &stream_;
};

/**
 * \class WarningMessage
 * \brief Class used to print a warning message in magenta using stream operators.
 *
 * Example:
 * \code{.cpp} WaringMessage() << "Warning at line" << 12 << endl; \endcode
 */
class WarningMessage {
public:
	WarningMessage(std::ostream &stream=std::cerr) : stream_(stream) {
		rlutil::setColor(rlutil::LIGHTMAGENTA);
		stream_ << "Warning: ";
		rlutil::resetColor();
	}

	WarningMessage(const clang::FullSourceLoc &loc, std::ostream &stream=std::cerr) : stream_(stream) {
		rlutil::setColor(rlutil::WHITE);
		stream_ << loc.printToString(loc.getManager()) << ": ";
		rlutil::setColor(rlutil::LIGHTMAGENTA);
		stream << "warning: ";
		rlutil::resetColor();
	}

	~WarningMessage() {
		rlutil::resetColor();
		stream_ << "\n\n";
	}

	template<class T>
	WarningMessage &operator<< (const T &right) {
		stream_ << right;
		return *this;
	}

private:
	std::ostream &stream_;
};

/**
 * Given as input a path to a file, returns the path to the directory of the file
 * and keep in path only the file name.
 */
std::string ExtractMainDirectory(std::string &path);

/**
 * Build every folder that should exist to have the file given in entry. If the folders already
 * exist, it does not create duplicate.
 */
void BuildFolders (std::string file);

/**
 * Get the relative path to the Assasim folder.
 */
std::string GetAssasimFolder(std::string executable_path);

/**
 * Copy a files entierely
 */

void CopyFiles(std::string from, std::string to);

/**
 * Contains a location as (FileID, LineNumber)
 */
typedef std::pair<clang::FileID, unsigned> PairLocation;

/**
 * Hash function to use unordered structures to remember FileID and line number
 */
struct hashPairLocation {
	size_t operator()(PairLocation p) const {
		return p.first.getHashValue() ^ std::hash<unsigned>()(p.second);
	}
};

/**
   Hash function for clang::SourceLocation
 */
struct hashSourceLocation {
	size_t operator()(clang::SourceLocation sl) const {
		return sl.getRawEncoding();
	}
};

#endif
