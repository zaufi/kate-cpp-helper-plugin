# - Integrate Boost unit tests into CMake infrastructure
#
# Usage:
#   add_boost_tests(
#       [TARGET executable]
#       [MODULE test-module-name]
#       SOURCES source1 [source2 ... sourceN]
#       [WORKING_DIRECTORY dir]
#       [CATCH_SYSTEM_ERRORS | NO_CATCH_SYSTEM_ERRORS]
#       [USE_HEADER_ONLY_VERSION]
#       [DYN_LINK]
#     )
#   executable -- name of unit tests binary (if omitted, `unit_tests' will be used)
#   source1-N  -- list of source files to be scanned for test cases
#   dir        -- `cd` to this directory before run any test
#
# If running under TeamCity, additional sources would be added allowing integration
# of Boost UTF and TeamCity reporting facilities.
# Use [`teamcity-cpp-boost`](https://github.com/zaufi/teamcity-cpp) if found, otherwise
# internal sources will be used.
#
# If `DYN_LINK` is not specified, `Boost_USE_STATIC_LIBS` would be checked additionaly
# and if latter is OFF, `DYN_LINK` will be used.
#
# Adding `add_boost_tests_DEBUG` to CMake CLI or environment will turn debug
# messages ON. Just like adding `DEBUG` option do for individual `add_boost_tests()`
# call.
#
# Source files will be scanned for automatic test cases.
# Every found test will be added to ctest list. Be aware that "scanner" is very
# limited (yep, it is not a full functional C++ parser). Particularly:
#  - `BOOST_AUTO_TEST_CASE`, `BOOST_FIXTURE_TEST_SUITE`, `BOOST_FIXTURE_TEST_CASE`,
#    `BOOST_AUTO_TEST_SUITE` and `BOOST_AUTO_TEST_SUITE_END` the only recognizable tokens
#  - every token must be at line start
#  - parameter list followed immediately after token name -- i.e. no space
#    between toked identifier and opening parenthesis
#  - parameters for listed macros are expected at the same line
# This function can be considered as 'improved' `add_executable(unit_tests <sources>)`
# to build boost based unit tests and integrate them into cmake's `make test`.
#

#=============================================================================
# Copyright 2011-2016 by Alex Turbov <i.zaufi@gmail.com>
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

include("${_ADD_BOOST_TEST_BASE_DIR}/TeamCityIntegration.cmake")

is_running_under_teamcity(_ADD_BOOST_TEST_UNDER_TEAM_CITY)

if(_ADD_BOOST_TEST_UNDER_TEAM_CITY)
    find_package(teamcity-cpp-boost 1.7.2 QUIET)
    # If found an object library target with same name will be defined
    if(TARGET teamcity-cpp-boost)
        set(_ADD_BOOST_TEST_WITH_TEAM_CITY ON)
    else()
        message(STATUS "Looking for TeamCity unit tests integration library (teamcity-cpp-boost) - not found")
    endif()
endif()

if(NOT ADD_BOOST_TESTS_DEBUG AND "$ENV{add_boost_tests_DEBUG}")
    set(ADD_BOOST_TESTS_DEBUG ON)
endif()

function(add_boost_tests)
    set(options DEBUG CATCH_SYSTEM_ERRORS NO_CATCH_SYSTEM_ERRORS USE_HEADER_ONLY_VERSION DYN_LINK)
    set(one_value_args WORKING_DIRECTORY TARGET MODULE)
    set(multi_value_args SOURCES)
    cmake_parse_arguments(add_boost_tests "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    # Enable debug mode if it was requested via CMake CLI or environment
    if(NOT add_boost_tests_DEBUG AND ADD_BOOST_TESTS_DEBUG)
        set(add_boost_tests_DEBUG ON)
    endif()

    # Prevent running w/o source files
    if(NOT add_boost_tests_SOURCES)
        message(FATAL_ERROR "No source files given to `add_boost_tests'")
    else()
        set(add_boost_tests_FILTERED_SOURCES)
        set(add_boost_tests_GE)
        foreach(source ${add_boost_tests_SOURCES})
            if(source MATCHES "^\\$<.*>$")
                list(APPEND add_boost_tests_GE "${source}")
                if(add_boost_tests_DEBUG)
                    message(STATUS "  [add_boost_tests] checking source name: ${source} -- looks like generator expression...")
                endif()
            else()
                list(APPEND add_boost_tests_FILTERED_SOURCES "${source}")
                if(add_boost_tests_DEBUG)
                    message(STATUS "  [add_boost_tests] checking source name: ${source} -- Ok")
                endif()
            endif()
        endforeach()
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

    if(add_boost_tests_USE_HEADER_ONLY_VERSION)
        set(BOOST_TEST_INCLUDE "boost/test/included/unit_test.hpp")
    else()
        set(BOOST_TEST_INCLUDE "boost/test/unit_test.hpp")
    endif()

    if(add_boost_tests_DYN_LINK)
        set(BOOST_TEST_DYN_LINK "1")
    elseif(NOT Boost_USE_STATIC_LIBS)
        set(BOOST_TEST_DYN_LINK "1")
    endif()

    if(add_boost_tests_MODULE)
        set(BOOST_TEST_MODULE "${add_boost_tests_MODULE}")
    else()
        set(BOOST_AUTO_TEST_MAIN "1")
    endif()

    configure_file(
        "${_ADD_BOOST_TEST_BASE_DIR}/unit_tests_main_skeleton.cc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/unit_tests_main.cc"
        @ONLY
      )

    # If TeamCity detected, add few more sources
    if(_ADD_BOOST_TEST_WITH_TEAM_CITY)
        if(add_boost_tests_DEBUG)
            message(STATUS "  [add_boost_tests] teamcity-cpp-boost has found")
        endif()
        set(tc_sources "\$<TARGET_OBJECTS:teamcity-cpp-boost>")
    elseif(_ADD_BOOST_TEST_UNDER_TEAM_CITY)
        if(add_boost_tests_DEBUG)
            message(STATUS "  [add_boost_tests] teamcity-cpp-boost not found. Will use bundled sources.")
        endif()
        if(NOT TEAMCITY_BOOST_SOURCES)
            set(
                TEAMCITY_BOOST_SOURCES
                "${_ADD_BOOST_TEST_BASE_DIR}/teamcity-boost/teamcity_boost.cpp"
                "${_ADD_BOOST_TEST_BASE_DIR}/teamcity-boost/teamcity_messages.cpp"
                "${_ADD_BOOST_TEST_BASE_DIR}/teamcity-boost/teamcity_messages.h"
              )
        endif()
        set(tc_sources "${TEAMCITY_BOOST_SOURCES}")
    endif()

    # Add unit tests executable target to current directory
    add_executable(
        ${add_boost_tests_TARGET}
        ${add_boost_tests_FILTERED_SOURCES}
        ${add_boost_tests_GE}
        ${tc_sources}
        "${CMAKE_CURRENT_BINARY_DIR}/unit_tests_main.cc"
      )

    # Add link target if Boost UTF has been found
    if(TARGET Boost::unit_test_framework)
        target_link_libraries(
            ${add_boost_tests_TARGET}
            Boost::unit_test_framework
          )
        # For CMake < 3.5
        if(Boost_UNIT_TEST_FRAMEWORK_LIBRARY)
            target_link_libraries(
                ${add_boost_tests_TARGET}
                ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
              )
        endif()
    endif()

    # Try to render templated header files
    file(GLOB _conf_files "${CMAKE_CURRENT_SOURCE_DIR}/*.h.in")
    foreach(_f IN LISTS _conf_files)
        get_filename_component(_fname "${_f}" NAME)
        string(REGEX REPLACE ".in$" "" _fname "${_fname}")
        configure_file("${_f}" "${CMAKE_CURRENT_BINARY_DIR}/${_fname}" @ONLY)
    endforeach()

    # Set `PROJECT_TEST_EXECUTABLES` global property to help identify full names of test executables.
    # It can be reported to a CI server (by a project's `CMakeLists.txt`), so it wouldn't be necessarry to specify
    # tests to run manually :-)
    if(WIN32)
        # In multi-configuration environment lets put a marker tag to be replaced
        # later w/ actual configuration name. To do so, one may use `string(CONFIGURE ...)`.
        # NOTE Do nothing, if no configurations has been specified!
        if(CMAKE_CONFIGURATION_TYPES)
            set(_cfg_placeholder "@CURRENT_CONFIGURATION@/")
            set(_need_to_set_registration_property ON)
        endif()
    else()
        # For single configuration generators, just register a pure path w/o any placeholders.
        set(_need_to_set_registration_property ON)
    endif()

    if(_need_to_set_registration_property)
        # TODO There is no easy way to access a `LOCATION` property of the target,
        # so use a workaround... build it from parts!
        set_property(
            GLOBAL APPEND PROPERTY
            PROJECT_TEST_EXECUTABLES "${CMAKE_CURRENT_BINARY_DIR}/${_cfg_placeholder}${add_boost_tests_TARGET}${CMAKE_EXECUTABLE_SUFFIX}"
          )
    endif()

    # Scan source files for well known boost test framework macros and add test by found name
    foreach(source ${add_boost_tests_FILTERED_SOURCES})
        # ATTENTION Do not scan unexistent (probably generated) and Qt/MOC sources
        get_source_file_property(full_source "${source}" LOCATION)
        if(source MATCHES "/moc_.*$")
            set(_skip_scan ON)
        elseif (NOT EXISTS "${full_source}")
            set(_skip_scan ON)
        else()
            set(_skip_scan OFF)
        endif()
        if(NOT _skip_scan)
            set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${source})

            if(add_boost_tests_DEBUG)
                message(STATUS "  [add_boost_tests] Checking source file '${source}'")
            endif()

            file(
                STRINGS "${source}" contents
                REGEX "^\\s*BOOST_(AUTO_TEST_(CASE|SUITE(_END)?)|FIXTURE_TEST_(CASE|SUITE))"
              )
            set(current_suite "<NONE>")
            set(found_tests "")
            foreach(line IN LISTS contents)
                if(add_boost_tests_DEBUG)
                    message(STATUS "  [add_boost_tests] line: '${line}'")
                endif()

                set(found_test)
                # Try BOOST_AUTO_TEST_CASE
                if(line MATCHES "BOOST_AUTO_TEST_CASE\\([A-Za-z0-9_]+\\)")
                    string(
                        REGEX REPLACE
                            "BOOST_AUTO_TEST_CASE\\(([A-Za-z0-9_]+)\\).*" "\\1"
                        found_test
                        "${line}"
                      )
                elseif(line MATCHES "BOOST_FIXTURE_TEST_CASE\\([A-Za-z_0-9]+, [A-Za-z_0-9:]+\\)")
                    string(
                        REGEX REPLACE
                            "BOOST_FIXTURE_TEST_CASE\\(([A-Za-z_0-9]+), [A-Za-z_0-9:]+\\).*"
                            "\\1"
                        found_test
                        "${line}"
                      )
                elseif(line MATCHES "BOOST_FIXTURE_TEST_SUITE\\([A-Za-z_0-9]+, [A-Za-z_0-9:]+\\)")
                    string(
                        REGEX REPLACE
                            "BOOST_FIXTURE_TEST_SUITE\\(([A-Za-z_0-9]+), [A-Za-z_0-9:]+\\).*"
                            "\\1"
                        current_suite
                        "${line}"
                      )
                elseif(line MATCHES "BOOST_AUTO_TEST_SUITE\\([A-Za-z_0-9]+\\)")
                    string(
                        REGEX REPLACE
                            "BOOST_AUTO_TEST_SUITE\\(([A-Za-z0-9_]+)\\).*" "\\1"
                        current_suite
                        "${line}"
                      )
                elseif(line MATCHES "BOOST_AUTO_TEST_SUITE_END\\(\\)")
                    set(current_suite "<NONE>")
                endif()

                if(add_boost_tests_DEBUG)
                    message(STATUS "  [add_boost_tests] current suite: '${current_suite}'")
                    message(STATUS "  [add_boost_tests] found test: '${found_test}'")
                endif()

                # Add if any test was found
                if(found_test)
                    if(current_suite STREQUAL "<NONE>")
                        list(APPEND found_tests ${found_test})
                    else()
                        list(APPEND found_tests "${current_suite}/${found_test}")
                    endif()
                endif()
            endforeach()

            # Register found tests
            foreach(test_name ${found_tests})
                if(add_boost_tests_DEBUG)
                    message(
                        STATUS
                        "  [add_boost_tests] add test[${add_boost_tests_TARGET}]: ${test_name}"
                      )
                endif()
                add_test(
                    NAME ${test_name}
                    COMMAND $<TARGET_FILE:${add_boost_tests_TARGET}> --run_test=${test_name} ${extra_args}
                    WORKING_DIRECTORY ${add_boost_tests_WORKING_DIRECTORY}
                  )
            endforeach()
        endif()
    endforeach()
endfunction(add_boost_tests)

# X-Chewy-RepoBase: https://raw.githubusercontent.com/mutanabbi/chewy-cmake-rep/master/
# X-Chewy-Path: AddBoostTests.cmake
# X-Chewy-Version: 5.8
# X-Chewy-Description: Integrate Boost unit tests into CMake infrastructure
# X-Chewy-AddonFile: TeamCityIntegration.cmake
# X-Chewy-AddonFile: unit_tests_main_skeleton.cc.in
# X-Chewy-AddonFile: teamcity-boost/teamcity_boost.cpp
# X-Chewy-AddonFile: teamcity-boost/teamcity_messages.cpp
# X-Chewy-AddonFile: teamcity-boost/teamcity_messages.h
