#include <iostream>
#include <mpi.h>

#include "user_interface.hpp"


int main(int argc, char* argv[]) {
	int provided;
	// MPI is told to handle threads
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <interface_token>\n";
		exit(1);
	}

	InitUserInterface(std::string(argv[1]));

	MPI_Finalize();
	return 0;
}
