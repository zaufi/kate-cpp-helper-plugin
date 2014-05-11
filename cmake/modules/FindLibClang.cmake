#
# Find clang C API library
#

#=============================================================================
# Copyright 2012-2013 by Alex Turbov <i.zaufi@gmail.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file LICENSE for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of this repository, substitute the full
#  License text for the above reference.)


include(FindPackageHandleStandardArgs)

if(LibClang_FIND_VERSION_EXACT)
    if(LibClang_FIND_VERSION_MAJOR AND LibClang_FIND_VERSION_MINOR)
        set(_llvm_config_programs llvm-config-${LibClang_FIND_VERSION_MAJOR}.${LibClang_FIND_VERSION_MINOR})
    else()
        message(FATAL_ERROR "EXACT version mutch requested, but no version specified!")
    endif()
else()
    set(_llvm_config_programs
        llvm-config-3.4
        llvm-config-3.3
        llvm-config-3.2
        llvm-config-3.1
        llvm-config-3.0
      )
endif()

# Try to find it via `llvm-config` if no location specified
# in a command line...
if(NOT (LLVM_INCLUDEDIR OR LLVM_LIBDIR))
    # Use `llvm-config` to locate libclang.so
    find_program(
        LLVM_CONFIG_EXECUTABLE
        NAMES ${_llvm_config_programs} llvm-config
      )
    if(LLVM_CONFIG_EXECUTABLE)
        message(STATUS "Found LLVM configuration tool: ${LLVM_CONFIG_EXECUTABLE}")

        # Get LLVM library dir
        execute_process(
            COMMAND ${LLVM_CONFIG_EXECUTABLE} --libdir
            OUTPUT_VARIABLE LLVM_LIBDIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
          )
        execute_process(
            COMMAND ${LLVM_CONFIG_EXECUTABLE} --includedir
            OUTPUT_VARIABLE LLVM_INCLUDEDIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
          )
    endif()
endif()

# Try to find libclang.so
# NOTE Try to find libclang even if `llvm-config` is not found!
# User still can specify its location via overriding LLVM_LIBDIR and LLVM_INCLUDEDIR!
find_library(
    LIBCLANG_LIBRARY
    clang
    PATH ${LLVM_LIBDIR}
  )

# Do version check only if library was actually found
if(LIBCLANG_LIBRARY)
    # Try to compile sample test which would output clang version
    try_run(
        _clang_get_version_run_result
        _clang_get_version_compile_result
        ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_LIST_DIR}/libclang_get_version.cpp
        CMAKE_FLAGS
            -DLINK_LIBRARIES:STRING=${LIBCLANG_LIBRARY}
            -DINCLUDE_DIRECTORIES:STRING=${LLVM_INCLUDEDIR}
        COMPILE_OUTPUT_VARIABLE _clang_get_version_compile_output
        RUN_OUTPUT_VARIABLE _clang_get_version_run_output
      )
    # Extract clang version. In my system this string look like:
    # "clang version 3.1 (branches/release_31)"
    # In Ubuntu 12.10 it looks like:
    # "Ubuntu clang version 3.0-6ubuntu3 (tags/RELEASE_30/final) (based on LLVM 3.0)"
    string(
        REGEX
        REPLACE ".*clang version ([23][^ \\-]+)[ \\-].*" "\\1"
        LIBCLANG_VERSION
        "${_clang_get_version_run_output}"
      )

    message(STATUS "Found Clang C API: ${LIBCLANG_LIBRARY} (version ${LIBCLANG_VERSION})")
endif()

find_package_handle_standard_args(
    LibClang
    REQUIRED_VARS LIBCLANG_LIBRARY
    VERSION_VAR LIBCLANG_VERSION
  )

# X-Chewy-RepoBase: https://raw.githubusercontent.com/mutanabbi/chewy-cmake-rep/master/
# X-Chewy-Path: FindLibClang.cmake
# X-Chewy-Version: 2.0
# X-Chewy-Description: Find clang C API library
# X-Chewy-AddonFile: libclang_get_version.cpp
