# - Integrate Boost unit tests into CMake infrastructure
#
# Usage:
#   add_boost_tests(
#       [TARGET executable]
#       SOURCES source1 [source2 ... sourceN]
#       [WORKING_DIRECTORY dir]
#       [CATCH_SYSTEM_ERRORS | NO_CATCH_SYSTEM_ERRORS]
#     )
#   executable -- name of unit tests binary (if ommited, `unit_tests' will be used)
#   source1-N  -- list of source files scanned to be scanned
#   dir        -- cd to this directory before run test
#
# Source files will be canned for automatic test cases.
# Every found test will be added to ctest list.
# This function can be considered as 'improved' add_executable(unit_tests <sources>).
#

#=============================================================================
# Copyright 2011-2013 by Alex Turbov <i.zaufi@gmail.com>
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

include(CMakeParseArguments)

function(add_boost_tests)
    set(options CATCH_SYSTEM_ERRORS NO_CATCH_SYSTEM_ERRORS)
    set(one_value_args WORKING_DIRECTORY TARGET)
    set(multi_value_args SOURCES)
    cmake_parse_arguments(add_boost_tests "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    # Prevent running w/o source files
    if(NOT add_boost_tests_SOURCES)
        message(FATAL_ERROR "No source files given to `add_boost_tests'")
    endif()

    # Check for target executable
    if(NOT add_boost_tests_TARGET)
        # Set default executable name
        set(add_boost_tests_TARGET unit_tests)
    endif()

    # Check for optional params
    if(add_boost_tests_CATCH_SYSTEM_ERRORS)
        set(extra_args "--catch_system_errors=y")
    endif()
    if(add_boost_tests_NO_CATCH_SYSTEM_ERRORS)
        if(NOT add_boost_tests_CATCH_SYSTEM_ERRORS)
            set(extra_args "--catch_system_errors=n")
        else()
            message(FATAL_ERROR "NO_CATCH_SYSTEM_ERRORS and CATCH_SYSTEM_ERRORS can't be specified together")
        endif()
    endif()

    if(NOT add_boost_tests_WORKING_DIRECTORY)
        set(work_dir ${CMAKE_CURRENT_BINARY_DIR})
    else()
        set(work_dir ${add_boost_tests_WORKING_DIRECTORY})
    endif()

    # Add unit tests executable target to current directory
    add_executable(${add_boost_tests_TARGET} ${add_boost_tests_SOURCES})

    # Scan source files for well known boost test framework macros and add test by found name
    foreach(source ${add_boost_tests_SOURCES})
        file(READ "${source}" contents)
        # Append BOOST_AUTO_TEST_CASEs
        string(REGEX MATCHALL "BOOST_AUTO_TEST_CASE\\(([A-Za-z0-9_]+)\\)" found_tests "${contents}")
        foreach(hit ${found_tests})
            string(REGEX REPLACE ".*\\(([A-Za-z0-9_]+)\\).*" "\\1" test_name ${hit})
            add_test(
                NAME ${test_name}
                COMMAND ${add_boost_tests_TARGET} --run_test=${test_name} ${extra_args}
                WORKING_DIRECTORY ${work_dir}
            )
        endforeach()
        # Append BOOST_FIXTURE_TEST_CASEs
        string(REGEX MATCHALL "BOOST_FIXTURE_TEST_SUITE\\(([A-Za-z_0-9]+), ([A-Za-z_0-9]+)\\)" found_tests "${contents}")
        foreach(hit ${found_tests})
            string(REGEX REPLACE ".*\\(([A-Za-z_0-9]+), ([A-Za-z_0-9]+)\\).*" "\\1" test_name ${hit})
            add_test(
                ${test_name}
                ${add_boost_tests_TARGET} --run_test=${test_name} ${extra_args}
                WORKING_DIRECTORY ${work_dir}
              )
        endforeach()
    endforeach()
endfunction(add_boost_tests)

# kate: hl cmake;
# X-Chewy-RepoBase: https://raw.github.com/mutanabbi/chewy-cmake-rep/master/
# X-Chewy-Path: AddBoostTests.cmake
# X-Chewy-Version: 2.2
# X-Chewy-Description: Integrate Boost unit tests into CMake infrastructure
