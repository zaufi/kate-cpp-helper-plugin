#
# Find clang C API libraries
#
# Copyright 2012 by Alex Turbov <i.zaufi@gmail.com>
#
# kate: hl cmake;

find_program(
    LLVM_CONFIG_EXECUTABLE
    llvm-config                                             # My gentoo system has this name
    llvm-config-3.1                                         # ???
    llvm-config-3.0                                         # Ubuntu 12.04 (quantal)
    REQUIRED
  )
message(STATUS "Found LLVM configuration tool: ${LLVM_CONFIG_EXECUTABLE}")

# Get LLVM CXX flags
execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --cxxflags
    OUTPUT_VARIABLE LLVM_CXXFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
# Remove undesirable flags
string(REPLACE "-DNDEBUG" "" LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")
string(
    REPLACE "-fno-exceptions" ""
    LLVM_CXXFLAGS
    "${LLVM_CXXFLAGS}"
  )

# Get LLVM library dir
execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --libdir
    OUTPUT_VARIABLE LLVM_LIBDIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
# Try to find libclang.so
find_library(
    CLANG_C_LIBRARIES
    clang
    PATH ${LLVM_LIBDIR}
  )
# Try to compile sample test which would output clang version
try_run(
    _clang_get_version_run_result
    _clang_get_version_compile_result
    ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/cmake/modules/clang-c-lib-get-version.cpp
    COMPILE_DEFINITIONS ${LLVM_CXXFLAGS}
    CMAKE_FLAGS -DLINK_LIBRARIES:STRING=${CLANG_C_LIBRARIES}
    COMPILE_OUTPUT_VARIABLE _clang_get_version_compile_output
    RUN_OUTPUT_VARIABLE _clang_get_version_run_output
  )
# Extract clang version. In my system this string look like:
# "clang version 3.1 (branches/release_31)"
string(REGEX REPLACE ".*([23][^ ]+) .*" "\\1" CLANG_C_LIB_VERSION "${_clang_get_version_run_output}")

message(STATUS "Found Clang C API version ${CLANG_C_LIB_VERSION}")

find_package_handle_standard_args(
    CLANG_C_LIBS
    REQUIRED_VARS LLVM_CXXFLAGS CLANG_C_LIBRARIES
    VERSION_VAR CLANG_C_LIB_VERSION
  )
