# - Integrate Boost unit tests into CMake infrastructure
#
# Usage:
#   add_boost_tests(
#       [TARGET executable]
#       SOURCES source1 [source2 ... sourceN]
#       [WORKING_DIRECTORY dir]
#       [CATCH_SYSTEM_ERRORS | NO_CATCH_SYSTEM_ERRORS]
#     )
#   executable -- name of unit tests binary (if omitted, `unit_tests' will be used)
#   source1-N  -- list of source files to be scanned for test cases
#   dir        -- `cd` to this directory before run any test
#
# Source files will be scanned for automatic test cases.
# Every found test will be added to ctest list. Be aware that "scanner" is very
# limited (yep, it is not a full functional C++ parser). Particularly:
#  - BOOST_AUTO_TEST_CASE, BOOST_FIXTURE_TEST_SUITE, BOOST_FIXTURE_TEST_CASE
#    and BOOST_AUTO_TEST_SUITE_END the only recognizable tokens
#  - every token must be at line start
#  - parameter list followed immediately after token name -- i.e. no space
#    between toked identifier and opening parenthesis
#  - parameters for listed macros are expected at the same line
# This function can be considered as 'improved' add_executable(unit_tests <sources>)
# to build boost based unit tests and integrate them into cmake's `make test`.
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

set(_ADD_BOOST_TEST_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")

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
        set(add_boost_tests_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()

    # Add unit tests executable target to current directory
    add_executable(
        ${add_boost_tests_TARGET}
        ${_ADD_BOOST_TEST_BASE_DIR}/unit_tests_main_skeleton.cc ${add_boost_tests_SOURCES}
      )

    # Scan source files for well known boost test framework macros and add test by found name
    foreach(source ${add_boost_tests_SOURCES})
        #message(STATUS "scanning ${source}")
        file(
            STRINGS "${source}" contents
            REGEX "^\\s*BOOST_(AUTO_TEST_(CASE|SUITE_END)|FIXTURE_TEST_(CASE|SUITE))"
          )
        set(current_suite "<NONE>")
        set(found_tests "")
        foreach(line IN LISTS contents)
            # Try BOOST_AUTO_TEST_CASE
            string(REGEX MATCH "BOOST_AUTO_TEST_CASE\\([A-Za-z0-9_]+\\)" found_test "${line}")
            if(found_test)
                string(REGEX REPLACE "BOOST_AUTO_TEST_CASE\\(([A-Za-z0-9_]+)\\)" "\\1" found_test "${line}")
                if(current_suite STREQUAL "<NONE>")
                    list(APPEND found_tests ${found_test})
                else()
                    list(APPEND found_tests "${current_suite}/${found_test}")
                endif()
            else()
                # Try BOOST_FIXTURE_TEST_CASE
                string(
                    REGEX MATCH
                        "BOOST_FIXTURE_TEST_CASE\\([A-Za-z_0-9]+, [A-Za-z_0-9]+\\)"
                    found_test
                    "${line}"
                  )
                if(found_test)
                    string(
                        REGEX REPLACE
                            "BOOST_FIXTURE_TEST_CASE\\(([A-Za-z_0-9]+), [A-Za-z_0-9]+\\)"
                            "\\1"
                        found_test
                        "${line}"
                      )
                    if(current_suite STREQUAL "<NONE>")
                        list(APPEND found_tests ${found_test})
                    else()
                        list(APPEND found_tests "${current_suite}/${found_test}")
                    endif()
                else()
                    string(
                        REGEX MATCH
                            "BOOST_FIXTURE_TEST_SUITE\\([A-Za-z_0-9]+, [A-Za-z_0-9]+\\)"
                        found_test
                        "${line}"
                        )
                    if(found_test)
                        string(
                            REGEX REPLACE
                                "BOOST_FIXTURE_TEST_SUITE\\(([A-Za-z_0-9]+), [A-Za-z_0-9]+\\)"
                                "\\1"
                            current_suite
                            "${line}"
                          )
                    elseif(line MATCHES "BOOST_AUTO_TEST_SUITE_END\\(\\)")
                        set(current_suite "<NONE>")
                    endif()
                endif()
            endif()
        endforeach()
        #message(STATUS "  found tests: ${found_tests}")
        # Register found tests
        foreach(test_name ${found_tests})
            add_test(
                NAME ${test_name}
                COMMAND ${add_boost_tests_TARGET} --run_test=${test_name} ${extra_args}
                WORKING_DIRECTORY ${add_boost_tests_WORKING_DIRECTORY}
              )
        endforeach()
    endforeach()
endfunction(add_boost_tests)

# X-Chewy-RepoBase: https://raw.github.com/mutanabbi/chewy-cmake-rep/master/
# X-Chewy-Path: AddBoostTests.cmake
# X-Chewy-Version: 3.1
# X-Chewy-Description: Integrate Boost unit tests into CMake infrastructure
# X-Chewy-AddonFile: unit_tests_main_skeleton.cc
