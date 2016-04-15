/**
 * \file utils.cpp
 * \brief Implements ExtractMainDirectory.
 */

#include <string>
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#include <llvm/Support/raw_ostream.h>

#include "utils.hpp"
std::string ExtractMainDirectory (std::string &path) {
	int index = path.length() - 1;
	while ((index >= 0) & (path[index] != '/')) {
		index --;
	}
	if (index < 0) {
		return "";
	} else {
		std::string directory_path = path.substr(0,index+1);
		path = path.substr(index+1, path.length()-index-1);
		return directory_path;
	}
}

//fonction pour créer des sous-dossier dans le dossier local si nécessaire
void BuildFolders (std::string file) {
	std::string output_folder = ExtractMainDirectory(file);
	boost::filesystem::path dir(output_folder);
	boost::filesystem::create_directories(dir);
}

std::string GetAssasimFolder(std::string executable_path) {
	std::string folder = ExtractMainDirectory(executable_path);
	folder = ExtractMainDirectory(folder); // remove /bin
	return "";
}

void CopyFiles(std::string from, std::string to) {
	for (boost::filesystem::directory_iterator file(from);
    file != boost::filesystem::directory_iterator(); ++file) {
		boost::filesystem::path current(file->path());
        std::string file_name = current.string();
        std::string origin = ExtractMainDirectory(file_name);

		std::string copied_file = to + file_name;
		if (boost::filesystem::is_regular_file(current)) {
			file_name = origin + file_name;
			boost::filesystem::remove(copied_file);
			boost::filesystem::copy_file(current, copied_file);
		} else {
			file_name = origin + file_name;
			BuildFolders(copied_file + "/");
			CopyFiles(file_name, copied_file + "/");
		}
	}
}
