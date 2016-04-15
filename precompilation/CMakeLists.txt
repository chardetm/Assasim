cmake_minimum_required(VERSION 2.6)
project(assasim-precompilation)

set(CMAKE_BUILD_TYPE Release)


# Find LLVM
if(NOT ("${LLVM_CONFIG}" MATCHES "^(llvm-config)(.*)$"))
  set(LLVM_CONFIG "llvm-config")
endif(NOT ("${LLVM_CONFIG}" MATCHES "^(llvm-config)(.*)$"))
message("LLVM config command: " ${LLVM_CONFIG})

# Find MPI
find_package(MPI REQUIRED)

# Find Boost
find_package(Boost COMPONENTS system filesystem REQUIRED)


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


# Find LLVM include directory
exec_program("${LLVM_CONFIG} --includedir" OUTPUT_VARIABLE LLVM_INCLUDES)
message("LLVM include directory: " ${LLVM_INCLUDES})
exec_program("${LLVM_CONFIG} --libdir" OUTPUT_VARIABLE LLVM_LIBS)
message("LLVM lib directory: " ${LLVM_LIBS})


# Add UbjsonCpp library
add_subdirectory(${CMAKE_SOURCE_DIR}/simulation_basis/libs/ubjsoncpp/src/ ${CMAKE_BINARY_DIR}/ubjsoncpp)


# Defines the required libraries for Clang
set (LIBS
    clangFrontend
    clangSerialization
    clangDriver
    clangTooling
    clangParse
    clangSema
    clangAnalysis
    clangRewriteFrontend
    clangEdit
    clangAST
    clangLex
    clangBasic
    clangCodeGen
    clangIndex
    clangStaticAnalyzerFrontend
    clangStaticAnalyzerCheckers
    clangStaticAnalyzerCore
    clangAnalysis
    clangRewriteFrontend
    clangRewrite

    LLVMOption
    LLVMMC
    LLVMObject
    LLVMBitReader
    LLVMAsmParser
    LLVMCore
    LLVMSupport
    LLVMMCParser

    tinfo
    z pthread ffi dl m
)


# Include directories
include_directories(${LLVM_INCLUDES} ${MPI_INCLUDE_PATH} ${Boost_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/simulation_basis/libs/ubjsoncpp/include/)
link_directories(${LLVM_LIBS})

# Source files
file(GLOB SOURCES src/*)

# Executable
set(
	EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/
)
add_executable(
	precompilation
    ${SOURCES}
)

# Library linking
target_link_libraries(
	precompilation
	UbjsonCpp
	${LIBS}
	${MPI_LIBRARIES}
	${Boost_LIBRARIES}
)
