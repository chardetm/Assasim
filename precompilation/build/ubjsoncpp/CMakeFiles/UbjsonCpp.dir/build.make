# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.5

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/chardetm/Projects/assasim/final/precompilation

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/chardetm/Projects/assasim/final/precompilation/build

# Include any dependencies generated for this target.
include ubjsoncpp/CMakeFiles/UbjsonCpp.dir/depend.make

# Include the progress variables for this target.
include ubjsoncpp/CMakeFiles/UbjsonCpp.dir/progress.make

# Include the compile flags for this target's objects.
include ubjsoncpp/CMakeFiles/UbjsonCpp.dir/flags.make

ubjsoncpp/CMakeFiles/UbjsonCpp.dir/value.cpp.o: ubjsoncpp/CMakeFiles/UbjsonCpp.dir/flags.make
ubjsoncpp/CMakeFiles/UbjsonCpp.dir/value.cpp.o: /home/chardetm/Projects/assasim/final/simulation_basis/libs/ubjsoncpp/src/value.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/chardetm/Projects/assasim/final/precompilation/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object ubjsoncpp/CMakeFiles/UbjsonCpp.dir/value.cpp.o"
	cd /home/chardetm/Projects/assasim/final/precompilation/build/ubjsoncpp && /usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/UbjsonCpp.dir/value.cpp.o -c /home/chardetm/Projects/assasim/final/simulation_basis/libs/ubjsoncpp/src/value.cpp

ubjsoncpp/CMakeFiles/UbjsonCpp.dir/value.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/UbjsonCpp.dir/value.cpp.i"
	cd /home/chardetm/Projects/assasim/final/precompilation/build/ubjsoncpp && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/chardetm/Projects/assasim/final/simulation_basis/libs/ubjsoncpp/src/value.cpp > CMakeFiles/UbjsonCpp.dir/value.cpp.i

ubjsoncpp/CMakeFiles/UbjsonCpp.dir/value.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/UbjsonCpp.dir/value.cpp.s"
	cd /home/chardetm/Projects/assasim/final/precompilation/build/ubjsoncpp && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/chardetm/Projects/assasim/final/simulation_basis/libs/ubjsoncpp/src/value.cpp -o CMakeFiles/UbjsonCpp.dir/value.cpp.s

ubjsoncpp/CMakeFiles/UbjsonCpp.dir/value.cpp.o.requires:

.PHONY : ubjsoncpp/CMakeFiles/UbjsonCpp.dir/value.cpp.o.requires

ubjsoncpp/CMakeFiles/UbjsonCpp.dir/value.cpp.o.provides: ubjsoncpp/CMakeFiles/UbjsonCpp.dir/value.cpp.o.requires
	$(MAKE) -f ubjsoncpp/CMakeFiles/UbjsonCpp.dir/build.make ubjsoncpp/CMakeFiles/UbjsonCpp.dir/value.cpp.o.provides.build
.PHONY : ubjsoncpp/CMakeFiles/UbjsonCpp.dir/value.cpp.o.provides

ubjsoncpp/CMakeFiles/UbjsonCpp.dir/value.cpp.o.provides.build: ubjsoncpp/CMakeFiles/UbjsonCpp.dir/value.cpp.o


# Object files for target UbjsonCpp
UbjsonCpp_OBJECTS = \
"CMakeFiles/UbjsonCpp.dir/value.cpp.o"

# External object files for target UbjsonCpp
UbjsonCpp_EXTERNAL_OBJECTS =

ubjsoncpp/libUbjsonCpp.so: ubjsoncpp/CMakeFiles/UbjsonCpp.dir/value.cpp.o
ubjsoncpp/libUbjsonCpp.so: ubjsoncpp/CMakeFiles/UbjsonCpp.dir/build.make
ubjsoncpp/libUbjsonCpp.so: ubjsoncpp/CMakeFiles/UbjsonCpp.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/chardetm/Projects/assasim/final/precompilation/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX shared library libUbjsonCpp.so"
	cd /home/chardetm/Projects/assasim/final/precompilation/build/ubjsoncpp && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/UbjsonCpp.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
ubjsoncpp/CMakeFiles/UbjsonCpp.dir/build: ubjsoncpp/libUbjsonCpp.so

.PHONY : ubjsoncpp/CMakeFiles/UbjsonCpp.dir/build

ubjsoncpp/CMakeFiles/UbjsonCpp.dir/requires: ubjsoncpp/CMakeFiles/UbjsonCpp.dir/value.cpp.o.requires

.PHONY : ubjsoncpp/CMakeFiles/UbjsonCpp.dir/requires

ubjsoncpp/CMakeFiles/UbjsonCpp.dir/clean:
	cd /home/chardetm/Projects/assasim/final/precompilation/build/ubjsoncpp && $(CMAKE_COMMAND) -P CMakeFiles/UbjsonCpp.dir/cmake_clean.cmake
.PHONY : ubjsoncpp/CMakeFiles/UbjsonCpp.dir/clean

ubjsoncpp/CMakeFiles/UbjsonCpp.dir/depend:
	cd /home/chardetm/Projects/assasim/final/precompilation/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/chardetm/Projects/assasim/final/precompilation /home/chardetm/Projects/assasim/final/simulation_basis/libs/ubjsoncpp/src /home/chardetm/Projects/assasim/final/precompilation/build /home/chardetm/Projects/assasim/final/precompilation/build/ubjsoncpp /home/chardetm/Projects/assasim/final/precompilation/build/ubjsoncpp/CMakeFiles/UbjsonCpp.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : ubjsoncpp/CMakeFiles/UbjsonCpp.dir/depend

