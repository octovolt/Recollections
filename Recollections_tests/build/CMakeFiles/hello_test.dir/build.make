# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.25

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
CMAKE_COMMAND = /usr/local/Cellar/cmake/3.25.1/bin/cmake

# The command to remove a file.
RM = /usr/local/Cellar/cmake/3.25.1/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/bfisher/Documents/Arduino/Recollections/Recollections_tests

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/bfisher/Documents/Arduino/Recollections/Recollections_tests/build

# Include any dependencies generated for this target.
include CMakeFiles/hello_test.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/hello_test.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/hello_test.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/hello_test.dir/flags.make

CMakeFiles/hello_test.dir/hello_test.cc.o: CMakeFiles/hello_test.dir/flags.make
CMakeFiles/hello_test.dir/hello_test.cc.o: /Users/bfisher/Documents/Arduino/Recollections/Recollections_tests/hello_test.cc
CMakeFiles/hello_test.dir/hello_test.cc.o: CMakeFiles/hello_test.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/bfisher/Documents/Arduino/Recollections/Recollections_tests/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/hello_test.dir/hello_test.cc.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/hello_test.dir/hello_test.cc.o -MF CMakeFiles/hello_test.dir/hello_test.cc.o.d -o CMakeFiles/hello_test.dir/hello_test.cc.o -c /Users/bfisher/Documents/Arduino/Recollections/Recollections_tests/hello_test.cc

CMakeFiles/hello_test.dir/hello_test.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/hello_test.dir/hello_test.cc.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/bfisher/Documents/Arduino/Recollections/Recollections_tests/hello_test.cc > CMakeFiles/hello_test.dir/hello_test.cc.i

CMakeFiles/hello_test.dir/hello_test.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/hello_test.dir/hello_test.cc.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/bfisher/Documents/Arduino/Recollections/Recollections_tests/hello_test.cc -o CMakeFiles/hello_test.dir/hello_test.cc.s

CMakeFiles/hello_test.dir/Utils_tests.cc.o: CMakeFiles/hello_test.dir/flags.make
CMakeFiles/hello_test.dir/Utils_tests.cc.o: /Users/bfisher/Documents/Arduino/Recollections/Recollections_tests/Utils_tests.cc
CMakeFiles/hello_test.dir/Utils_tests.cc.o: CMakeFiles/hello_test.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/bfisher/Documents/Arduino/Recollections/Recollections_tests/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/hello_test.dir/Utils_tests.cc.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/hello_test.dir/Utils_tests.cc.o -MF CMakeFiles/hello_test.dir/Utils_tests.cc.o.d -o CMakeFiles/hello_test.dir/Utils_tests.cc.o -c /Users/bfisher/Documents/Arduino/Recollections/Recollections_tests/Utils_tests.cc

CMakeFiles/hello_test.dir/Utils_tests.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/hello_test.dir/Utils_tests.cc.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/bfisher/Documents/Arduino/Recollections/Recollections_tests/Utils_tests.cc > CMakeFiles/hello_test.dir/Utils_tests.cc.i

CMakeFiles/hello_test.dir/Utils_tests.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/hello_test.dir/Utils_tests.cc.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/bfisher/Documents/Arduino/Recollections/Recollections_tests/Utils_tests.cc -o CMakeFiles/hello_test.dir/Utils_tests.cc.s

# Object files for target hello_test
hello_test_OBJECTS = \
"CMakeFiles/hello_test.dir/hello_test.cc.o" \
"CMakeFiles/hello_test.dir/Utils_tests.cc.o"

# External object files for target hello_test
hello_test_EXTERNAL_OBJECTS =

hello_test: CMakeFiles/hello_test.dir/hello_test.cc.o
hello_test: CMakeFiles/hello_test.dir/Utils_tests.cc.o
hello_test: CMakeFiles/hello_test.dir/build.make
hello_test: lib/libgtest_main.a
hello_test: lib/libgtest.a
hello_test: CMakeFiles/hello_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/bfisher/Documents/Arduino/Recollections/Recollections_tests/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable hello_test"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/hello_test.dir/link.txt --verbose=$(VERBOSE)
	/usr/local/Cellar/cmake/3.25.1/bin/cmake -D TEST_TARGET=hello_test -D TEST_EXECUTABLE=/Users/bfisher/Documents/Arduino/Recollections/Recollections_tests/build/hello_test -D TEST_EXECUTOR= -D TEST_WORKING_DIR=/Users/bfisher/Documents/Arduino/Recollections/Recollections_tests/build -D TEST_EXTRA_ARGS= -D TEST_PROPERTIES= -D TEST_PREFIX= -D TEST_SUFFIX= -D TEST_FILTER= -D NO_PRETTY_TYPES=FALSE -D NO_PRETTY_VALUES=FALSE -D TEST_LIST=hello_test_TESTS -D CTEST_FILE=/Users/bfisher/Documents/Arduino/Recollections/Recollections_tests/build/hello_test[1]_tests.cmake -D TEST_DISCOVERY_TIMEOUT=5 -D TEST_XML_OUTPUT_DIR= -P /usr/local/Cellar/cmake/3.25.1/share/cmake/Modules/GoogleTestAddTests.cmake

# Rule to build all files generated by this target.
CMakeFiles/hello_test.dir/build: hello_test
.PHONY : CMakeFiles/hello_test.dir/build

CMakeFiles/hello_test.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/hello_test.dir/cmake_clean.cmake
.PHONY : CMakeFiles/hello_test.dir/clean

CMakeFiles/hello_test.dir/depend:
	cd /Users/bfisher/Documents/Arduino/Recollections/Recollections_tests/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/bfisher/Documents/Arduino/Recollections/Recollections_tests /Users/bfisher/Documents/Arduino/Recollections/Recollections_tests /Users/bfisher/Documents/Arduino/Recollections/Recollections_tests/build /Users/bfisher/Documents/Arduino/Recollections/Recollections_tests/build /Users/bfisher/Documents/Arduino/Recollections/Recollections_tests/build/CMakeFiles/hello_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/hello_test.dir/depend

