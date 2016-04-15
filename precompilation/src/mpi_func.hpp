/**
 * \file mpi_func.hpp
 * \brief Contains the utilities regarding MPI_Datatypes creation.
 */

#ifndef MPI_FUNC_HPP_
#define MPI_FUNC_HPP_

#include <sstream>
#include <unordered_set>
#include <mpi.h>
#include "analyze_class.hpp"
#include "clang/AST/Type.h"

/**
 * \class MPITypeMap
 * \brief Contains the map for MPI integral type conversion.
 */

class MPITypeMap {
public:
	static MPITypeMap *instance;
	static MPITypeMap &GetInstance() {
		if (instance == nullptr)
			instance = new MPITypeMap();
		return *instance;
	}

	MPITypeMap() {
		//instance = nullptr;
		// Initializing the map
		integral_map_["char"] = "MPI_CHAR";
		integral_map_["wchar_t"] = "MPI_WCHAR";
		integral_map_["short"] = "MPI_SHORT";
		integral_map_["int"] = "MPI_INT";
		integral_map_["long"] = "MPI_LONG";
		integral_map_["long long int"] = "MPI_LONG_LONG_INT";
		integral_map_["long long"] = "MPI_LONG_LONG";
		integral_map_["signed char"] = "MPI_SIGNED_CHAR";
		integral_map_["unsigned char"] = "MPI_UNSIGNED_CHAR";
		integral_map_["unsigned short"] = "MPI_UNSIGNED_SHORT";
		integral_map_["unsigned long"] = "MPI_UNSIGNED_LONG";
		integral_map_["unsigned"] = "MPI_UNSIGNED";
		integral_map_["float"] = "MPI_FLOAT";
		integral_map_["double"] = "MPI_DOUBLE";
		integral_map_["long double"] = "MPI_LONG_DOUBLE";
		integral_map_["bool"] = "MPI_C_BOOL";
		integral_map_["_Bool"] = "MPI_C_BOOL";
		integral_map_["int8_t"] = "MPI_INT8_T";
		integral_map_["int16_t"] = "MPI_INT16_T";
		integral_map_["int32_t"] = "MPI_INT32_T";
		integral_map_["int64_t"] = "MPI_INT64_T";
		integral_map_["uint8_t"] = "MPI_UINT8_T";
		integral_map_["uint16_t"] = "MPI_UINT16_T";
		integral_map_["uint32_t"] = "MPI_UINT32_T";
		integral_map_["uint64_t"] = "MPI_UINT64_T";
	}

	static void Free() {
		if (instance != nullptr)
			delete instance;
		instance = nullptr;
	}

	std::string GetIntegralType(std::string stype) {
		return integral_map_.at(stype);
	}

	size_t IntegralCount(std::string s) {
		return integral_map_.count(s);
	}

private:
	std::map<std::string, std::string> integral_map_;
};

/// Generates the code loading the MPIDatatype corresponding to type (if it is of structural type).
/// The result is stored in temporary named temp
std::string GenerateCodeMPIDatatype(const clang::QualType &type,
	const clang::ASTContext *context, std::string temp, std::unordered_set<std::string> &temp_database);

#endif
