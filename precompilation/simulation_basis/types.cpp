#include <mpi.h>

#include "types.hpp"


void generateMPIDatatype(MPI_Datatype &type) {
	int MetaEvolutionDescriptionBlockLength[5] = {1, 1, 1, 1, 1};

	MPI_Aint MetaEvolutionDescriptionOffsets[5] = {
		offsetof(MetaEvolutionDescription, type),
		offsetof(MetaEvolutionDescription, agent_id),
		offsetof(MetaEvolutionDescription, origin_id),
		offsetof(MetaEvolutionDescription, destination_id),
		offsetof(MetaEvolutionDescription, private_overhead)
	};
	MPI_Datatype MetaEvolutionDescriptionFields[5] = {MPI_INT, MPI_UINT64_T, MPI_INT, MPI_INT, MPI_INT};

	MPI_Type_create_struct(5, MetaEvolutionDescriptionBlockLength, MetaEvolutionDescriptionOffsets, MetaEvolutionDescriptionFields, &type);
	MPI_Type_commit(&type);
}
