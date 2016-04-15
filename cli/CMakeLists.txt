cmake_minimum_required(VERSION 2.6)
project(assasim-cli)

set(CMAKE_BUILD_TYPE Release)


# Find MPI
find_package(MPI REQUIRED)

# Find readline
find_path(Readline_ROOT_DIR
    NAMES include/readline/readline.h
)
find_path(Readline_INCLUDE_DIR
    NAMES readline/readline.h
    HINTS ${Readline_ROOT_DIR}/include
)
find_library(Readline_LIBRARY
    NAMES readline
    HINTS ${Readline_ROOT_DIR}/lib
)
if(Readline_INCLUDE_DIR AND Readline_LIBRARY AND Ncurses_LIBRARY)
  set(READLINE_FOUND TRUE)
else(Readline_INCLUDE_DIR AND Readline_LIBRARY AND Ncurses_LIBRARY)
  FIND_LIBRARY(Readline_LIBRARY NAMES readline)
  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Readline DEFAULT_MSG Readline_INCLUDE_DIR Readline_LIBRARY )
  MARK_AS_ADVANCED(Readline_INCLUDE_DIR Readline_LIBRARY)
endif(Readline_INCLUDE_DIR AND Readline_LIBRARY AND Ncurses_LIBRARY)
mark_as_advanced(
    Readline_ROOT_DIR
    Readline_INCLUDE_DIR
    Readline_LIBRARY
)


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

# File inclusion
include_directories(${MPI_INCLUDE_PATH} ${Boost_INCLUDE_DIR} ${Readline_INCLUDE_DIR})

# Source files
file(GLOB SOURCES command_line_interface.cpp)

# Declaration of the executable
add_executable(
	assasim-cli
	${SOURCES}
)

# Library linking
target_link_libraries(
	assasim-cli
	${MPI_LIBRARIES}
	${Boost_LIBRARIES}
	${Readline_LIBRARY}
	rt
	pthread
)
