# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.28

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
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
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/zzy/c++/毕业设计/app-/server

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/zzy/c++/毕业设计/app-/server/build

# Include any dependencies generated for this target.
include CMakeFiles/Server.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/Server.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/Server.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/Server.dir/flags.make

CMakeFiles/Server.dir/src/main.cpp.o: CMakeFiles/Server.dir/flags.make
CMakeFiles/Server.dir/src/main.cpp.o: /home/zzy/c++/毕业设计/app-/server/src/main.cpp
CMakeFiles/Server.dir/src/main.cpp.o: CMakeFiles/Server.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/zzy/c++/毕业设计/app-/server/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/Server.dir/src/main.cpp.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/Server.dir/src/main.cpp.o -MF CMakeFiles/Server.dir/src/main.cpp.o.d -o CMakeFiles/Server.dir/src/main.cpp.o -c /home/zzy/c++/毕业设计/app-/server/src/main.cpp

CMakeFiles/Server.dir/src/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/Server.dir/src/main.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zzy/c++/毕业设计/app-/server/src/main.cpp > CMakeFiles/Server.dir/src/main.cpp.i

CMakeFiles/Server.dir/src/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/Server.dir/src/main.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zzy/c++/毕业设计/app-/server/src/main.cpp -o CMakeFiles/Server.dir/src/main.cpp.s

# Object files for target Server
Server_OBJECTS = \
"CMakeFiles/Server.dir/src/main.cpp.o"

# External object files for target Server
Server_EXTERNAL_OBJECTS =

Server: CMakeFiles/Server.dir/src/main.cpp.o
Server: CMakeFiles/Server.dir/build.make
Server: CMakeFiles/Server.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/zzy/c++/毕业设计/app-/server/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable Server"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/Server.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/Server.dir/build: Server
.PHONY : CMakeFiles/Server.dir/build

CMakeFiles/Server.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/Server.dir/cmake_clean.cmake
.PHONY : CMakeFiles/Server.dir/clean

CMakeFiles/Server.dir/depend:
	cd /home/zzy/c++/毕业设计/app-/server/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/zzy/c++/毕业设计/app-/server /home/zzy/c++/毕业设计/app-/server /home/zzy/c++/毕业设计/app-/server/build /home/zzy/c++/毕业设计/app-/server/build /home/zzy/c++/毕业设计/app-/server/build/CMakeFiles/Server.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/Server.dir/depend

