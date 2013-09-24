# - Define targets to generate skeleton files.
# This can be considered as a CLI based replacement for 'wizards' in various IDEs.
#
# It is possible to generate skeleton files for C++ header/implementation files
# and for boost::test based unit-tests.
#
# To make a new skeleton class one may use the following command:
#   $ cd <project-build-dir>
#   $ make new-class class=name subdir=project/source/relative/path ns=vendor::lib
#
# To generate unit tests skeleton:
#   $ cd <project-build-dir>
#   $ make new-class-tester class=name subdir=relative/path/to/tests
#
# NOTE GNU Autogen used internally.
#
# TODO Rewrite this stuff using Python + jinja2
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

set(_DSGT_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")

# Check if `autogen` and `awk` both are installed
find_program(AUTOGEN_EXECUTABLE autogen)


function(define_skeleton_generation_targets)
    if(NOT AUTOGEN_EXECUTABLE)
        message(STATUS "WARNING: You need to install GNU Autogen to be able to produce new C++ sources from skeletons")
    endif()

    # Parse function arguments
    set(options USE_CAMEL_STYLE ENABLE_TESTS USE_PRAGMA_ONCE)
    set(one_value_args HEADER_EXT IMPL_EXT PROJECT_PREFIX PROJECT_LICENSE PROJECT_NAMESPACE PROJECT_OWNER PROJECT_YEARS)
    set(multi_value_args)
    cmake_parse_arguments(define_skeleton_generation_targets "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    # Check mandatory arguments
    if(NOT define_skeleton_generation_targets_PROJECT_NAMESPACE)
        message(FATAL_ERROR "PROJECT_NAMESPACE is not ptovided")
        return()
    endif()
    # TODO Validate namespace spelling?
    set(PROJECT_DEFAULT_NAMESPACE "${define_skeleton_generation_targets_PROJECT_NAMESPACE}")

    # Check optional arguments and assign defaults of ommited
    if(define_skeleton_generation_targets_USE_CAMEL_STYLE)
        set(NAMING_STYLE "Camel")
    endif()
    #
    if(define_skeleton_generation_targets_USE_PRAGMA_ONCE)
        set(USE_PRAGMA_ONCE "yes")
    else()
        set(USE_PRAGMA_ONCE "no")
    endif()
    #
    if(define_skeleton_generation_targets_HEADER_EXT)
        set(SKELETON_HEADER_EXT "${define_skeleton_generation_targets_HEADER_EXT}")
    else()
        set(SKELETON_HEADER_EXT "hh")
    endif()
    #
    if(define_skeleton_generation_targets_IMPL_EXT)
        set(SKELETON_IMPL_EXT "${define_skeleton_generation_targets_IMPL_EXT}")
    else()
        set(SKELETON_IMPL_EXT "cc")
    endif()
    #
    if(define_skeleton_generation_targets_PROJECT_OWNER)
        set(PROJECT_OWNER "${define_skeleton_generation_targets_PROJECT_OWNER}")
    else()
        set(PROJECT_OWNER "respectful authors listed in AUTHORS file")
    endif()
    if(define_skeleton_generation_targets_PROJECT_YEARS)
        set(PROJECT_YEARS "${define_skeleton_generation_targets_PROJECT_YEARS}")
    else()
        # Get current year
        execute_process(
            COMMAND date "+%G"
            OUTPUT_VARIABLE PROJECT_YEARS
            OUTPUT_STRIP_TRAILING_WHITESPACE
          )
    endif()
    if(define_skeleton_generation_targets_PROJECT_PREFIX)
        set(PROJECT_PREFIX "${define_skeleton_generation_targets_PROJECT_PREFIX}")
    else()
        set(PROJECT_PREFIX "${PROJECT_NAME}")
    endif()

    set(NO_LICENSE "no")
    if(NOT define_skeleton_generation_targets_PROJECT_LICENSE)
        set(NO_LICENSE "yes")
    elseif(define_skeleton_generation_targets_PROJECT_LICENSE STREQUAL "GPL")
        set(PROJECT_LICENSE "gpl")
    elseif(define_skeleton_generation_targets_PROJECT_LICENSE STREQUAL "LGPL")
        set(PROJECT_LICENSE "lgpl")
    elseif(define_skeleton_generation_targets_PROJECT_LICENSE STREQUAL "BSD")
        set(PROJECT_LICENSE "bsd")
    else()
        set(PROJECT_LICENSE "${define_skeleton_generation_targets_PROJECT_LICENSE}")
    endif()

    # render template defaults
    configure_file(
        ${_DSGT_BASE_DIR}/tpl_defaults.def.in
        ${CMAKE_BINARY_DIR}/tpl_defaults.def
        @ONLY
      )
    # render files template
    configure_file(
        ${_DSGT_BASE_DIR}/class.tpl.in
        ${CMAKE_BINARY_DIR}/class.tpl
        @ONLY
      )
    # prepare autogen wrapper for new class skeleton creation
    configure_file(
        ${_DSGT_BASE_DIR}/new_class_wrapper.sh.in
        ${CMAKE_BINARY_DIR}/new_class_wrapper.sh
        @ONLY
      )

    # add new-class as target
    add_custom_target(
        new-class
        COMMAND /bin/sh ${CMAKE_BINARY_DIR}/new_class_wrapper.sh
        DEPENDS ${CMAKE_BINARY_DIR}/new_class_wrapper.sh
                ${CMAKE_BINARY_DIR}/class.tpl
                ${_DSGT_BASE_DIR}/class.tpl.in
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generate skeleton files"
      )

    if(define_skeleton_generation_targets_ENABLE_TESTS)
        configure_file(
            ${_DSGT_BASE_DIR}/class_tester.tpl.in
            ${CMAKE_BINARY_DIR}/class_tester.tpl
            @ONLY
          )
        configure_file(
            ${_DSGT_BASE_DIR}/new_class_tester_wrapper.sh.in
            ${CMAKE_BINARY_DIR}/new_class_tester_wrapper.sh
            @ONLY
          )
        # add new-class-tester as target
        add_custom_target(
            new-class-tester
            COMMAND /bin/sh ${CMAKE_BINARY_DIR}/new_class_tester_wrapper.sh
            DEPENDS ${CMAKE_BINARY_DIR}/new_class_tester_wrapper.sh
                    ${CMAKE_BINARY_DIR}/class_tester.tpl
                    ${_DSGT_BASE_DIR}/class_tester.tpl.in
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generate unit-test skeleton file"
          )
    endif()
endfunction()

# X-Chewy-RepoBase: https://raw.github.com/mutanabbi/chewy-cmake-rep/master/
# X-Chewy-Path: DefineSkeletonGenerationTargetsIfPossible.cmake
# X-Chewy-Version: 5.10
# X-Chewy-Description: Add targets to generate class header/implementation and unit-tests skeleton files
# X-Chewy-AddonFile: TestCMakeLists.txt.in
# X-Chewy-AddonFile: class.tpl.in
# X-Chewy-AddonFile: class_tester.tpl.in
# X-Chewy-AddonFile: mknslist.awk
# X-Chewy-AddonFile: new_class_tester_wrapper.sh.in
# X-Chewy-AddonFile: new_class_wrapper.sh.in
# X-Chewy-AddonFile: output_helpers.sh
# X-Chewy-AddonFile: tpl_defaults.def.in
