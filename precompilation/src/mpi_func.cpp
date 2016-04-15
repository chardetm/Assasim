#include "analyze_class.hpp"
#include "mpi_func.hpp"
#include "utils.hpp"

MPITypeMap *MPITypeMap::instance = nullptr;

std::string GenerateCodeMPIDatatype(const clang::QualType &type, const clang::ASTContext *context, std::string temp, std::unordered_set<std::string> &temp_database) {
	std::stringstream stream;
	MPITypeMap mpi_map = MPITypeMap::GetInstance();
	std::string name = type.getCanonicalType().getAsString();

	if (type.getCanonicalType().getTypePtr()->isBuiltinType() || type.getCanonicalType().getTypePtr()->isEnumeralType()) {
		// If it is integral type, returns the corresponding MPI_Datatype
		if (type.getCanonicalType().getTypePtr()->isSignedIntegerOrEnumerationType() && type.getCanonicalType().getTypePtr()->isUnsignedIntegerOrEnumerationType()) {
			// Treat enumerations as integers
			stream << mpi_map.GetIntegralType("int");
		} else if (!mpi_map.IntegralCount(name)) {
			ErrorMessage() << "builtin type " << name << " has no known corresponding MPI_Datatype";
			// Error (output empty string)
		} else
			stream << mpi_map.GetIntegralType(name); // Just return the corresponding type
	} else if (type.getCanonicalType().getTypePtr()->isStructureType()) {
		if (!temp_database.count(temp)) {
			stream << "\tMPI_Datatype " << temp << ";\n";
			temp_database.insert(temp);
		}
		clang::CXXRecordDecl *declaration = GetDeclarationOfClass(type);
		int n_fields = std::distance(declaration->field_begin(), declaration->field_end()); // Number of fields

		// Constructing the MPI datatype of the structure
		int i = 0;
		std::vector<int> lengths (n_fields,0);
		std::vector<MPI_Aint> offsets(n_fields,0);
		std::vector<std::string> type_temporaries = std::vector<std::string>(n_fields,"MPI_DATATYPE_NULL");
		// Recursively construct the data types of the fields
		for (const auto *field : declaration->fields()) {
			lengths[i] = 1;
			offsets[i] = context->getFieldOffset(field) / 8;
			std::string code_field = GenerateCodeMPIDatatype(field->getType(), context, temp + std::to_string(i), temp_database);
			if (code_field.substr(0,6) != "MPI_Da" && code_field.substr(0,3) == "MPI") // No temporary to use
				type_temporaries[i] = code_field;
			else {
				type_temporaries[i] = temp + std::to_string(i);
				stream << code_field;
			}

			i++;
		}
		stream << "\tlengths = {";
		for (int j = 0; j < n_fields; j++) {
			stream << lengths[j] << ",";
		}
		stream.seekp(-1,std::ios_base::cur);
		stream << "};\n";

		stream << "\toffsets = {";
		for (int j = 0; j < n_fields; j++) {
			stream << offsets[j] << ",";
		}
		stream.seekp(-1,std::ios_base::cur);
		stream << "};\n";

		stream << "\tmpi_types = {";
		for (int j = 0; j < n_fields; j++) {
			stream << type_temporaries[j] << ",";
		}
		stream.seekp(-1,std::ios_base::cur);
		stream << "};\n";

		stream << "\tMPI_Type_create_struct(" << n_fields
			   << ", lengths.data(), offsets.data(), mpi_types.data(), &" << temp << ");\n"
			   << "\tMPI_Type_commit(&" << temp << ");\n";
		// Now free the intermediary MPI_Datatypes
		for (const auto &temporary : type_temporaries) {
			if (temporary.length() > temp.length() && temporary.substr(0,temp.length()) == temp) // if it represents a constructed MPI_Datatype, free it
				stream << "\tMPI_Type_free(&" << temporary <<");\n";
		}
	} else {
		ErrorMessage() << name << " is not of structural type";
	}

	return stream.str();
}
