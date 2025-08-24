# CMake generated Testfile for 
# Source directory: /workspace
# Build directory: /workspace/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[scan_tests]=] "/workspace/build/scan_tests")
set_tests_properties([=[scan_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/workspace/CMakeLists.txt;34;add_test;/workspace/CMakeLists.txt;0;")
subdirs("_deps/googletest-build")
