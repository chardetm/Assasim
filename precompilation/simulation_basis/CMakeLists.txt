cmake_minimum_required(VERSION 2.6)
project(assasim-simulation)

set(CMAKE_BUILD_TYPE Release)


# Find MPI
find_package(MPI REQUIRED)

# Find C++ threads library
find_package(Threads REQUIRED)


# Verification of the support of C++14
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
CHECK_CXX_COMPILER_FLAG("-std=c++1y" COMPILER_SUPPORTS_CXX1Y)
if(COMPILER_SUPPORTS_CXX14)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
elseif(COMPILER_SUPPORTS_CXX1Y)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support. Please use a different C++ compiler.")
endif()


# Add UbjsonCpp library
add_subdirectory(${CMAKE_SOURCE_DIR}/libs/ubjsoncpp/src/ ${CMAKE_BINARY_DIR}/ubjsoncpp)


# Include directories
include_directories(
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/utils
	${CMAKE_SOURCE_DIR}/libs/ubjsoncpp/include
	${MPI_INCLUDE_PATH}
)

# Source files
file(GLOB SOURCES "*.cpp")

# Executable
set(
	EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin
)
add_executable(
	assasim-simulation
	${SOURCES}
)

# Library linking
target_link_libraries(
	assasim-simulation
	UbjsonCpp
	${MPI_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
	rt
)
