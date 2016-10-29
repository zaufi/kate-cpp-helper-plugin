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
# TODO Rewrite this stuff using Python + jinja2 (really?)
# TODO How to deal w/ dependency on output_helpers.sh, which
# is required by two modules??!
#

#=============================================================================
# Copyright 2011-2014 by Alex Turbov <i.zaufi@gmail.com>
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

# Check if `autogen` installed
find_program(AUTOGEN_EXECUTABLE autogen)


function(define_skeleton_generation_targets)
    if(NOT AUTOGEN_EXECUTABLE)
        message(
            STATUS
            "WARNING: You need to install GNU Autogen to be able to produce new C++ sources from skeletons"
          )
    endif()
    if(NOT CMAKE_GENERATOR STREQUAL "Unix Makefiles")
        message(
            STATUS
            "WARNING: Skeleton generation targets are reasonable for Unix Makefiles only (nowadays), so disabled"
          )
        return()
    endif()
    # Parse function arguments
    set(options ENABLE_TESTS USE_PRAGMA_ONCE)
    set(
        one_value_args
            HEADER_EXT
            IMPL_EXT
            TEST_FILE_SUFFIX
            TEST_NAME_SUFFIX
            PROJECT_PREFIX
            PROJECT_LICENSE
            PROJECT_NAMESPACE
            PROJECT_OWNER
            PROJECT_YEARS
      )
    set(multi_value_args)
    cmake_parse_arguments(
        define_skeleton_generation_targets
        "${options}"
        "${one_value_args}"
        "${multi_value_args}"
        ${ARGN}
      )

    # Check mandatory arguments
    if(NOT define_skeleton_generation_targets_PROJECT_NAMESPACE)
        message(FATAL_ERROR "PROJECT_NAMESPACE is not provided")
        return()
    endif()
    # TODO Validate namespace spelling?
    set(PROJECT_DEFAULT_NAMESPACE "${define_skeleton_generation_targets_PROJECT_NAMESPACE}")

    # Check optional arguments and assign defaults of ommited
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
        string(TIMESTAMP PROJECT_YEARS "%Y")
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
    elseif(define_skeleton_generation_targets_PROJECT_LICENSE STREQUAL "GPL-2")
        set(PROJECT_LICENSE "gplv2")
    elseif(define_skeleton_generation_targets_PROJECT_LICENSE STREQUAL "LGPL")
        set(PROJECT_LICENSE "lgpl")
    elseif(define_skeleton_generation_targets_PROJECT_LICENSE STREQUAL "LGPL-2")
        set(PROJECT_LICENSE "lgplv2")
    elseif(define_skeleton_generation_targets_PROJECT_LICENSE STREQUAL "BSD")
        set(PROJECT_LICENSE "bsd")
    else()
        set(PROJECT_LICENSE "${define_skeleton_generation_targets_PROJECT_LICENSE}")
    endif()

    set(_template "class.tpl")
    # render files template
    configure_file(
        ${_DSGT_BASE_DIR}/class.tpl.in
        ${CMAKE_CURRENT_BINARY_DIR}/class.tpl
        @ONLY
      )
    # prepare autogen wrapper for new class skeleton creation
    configure_file(
        ${_DSGT_BASE_DIR}/render_template.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/new_class.cmake
        @ONLY
      )

    # add new-class as target
    add_custom_target(
        new-class
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/new_class.cmake
        DEPENDS
            ${CMAKE_CURRENT_BINARY_DIR}/new_class.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/class.tpl
            ${_DSGT_BASE_DIR}/class.tpl.in
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generate skeleton files"
      )

    if(define_skeleton_generation_targets_ENABLE_TESTS)
        set(_template "class_tester.tpl")
        if(define_skeleton_generation_targets_TEST_FILE_SUFFIX)
            set(_filename_suffix "${define_skeleton_generation_targets_TEST_FILE_SUFFIX}")
        else()
            set(_filename_suffix "_tester")
        endif()
        if(define_skeleton_generation_targets_TEST_NAME_SUFFIX)
            set(TESTNAME_SUFFIX "${define_skeleton_generation_targets_TEST_NAME_SUFFIX}")
        else()
            set(TESTNAME_SUFFIX "_test")
        endif()

        configure_file(
            ${_DSGT_BASE_DIR}/class_tester.tpl.in
            ${CMAKE_CURRENT_BINARY_DIR}/class_tester.tpl
            @ONLY
          )
        configure_file(
            ${_DSGT_BASE_DIR}/render_template.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/new_class_tester.cmake
            @ONLY
          )
        configure_file(
            ${_DSGT_BASE_DIR}/new_class_tester_post.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/new_class_tester_post.cmake
            @ONLY
          )
        # add new-class-tester as target
        add_custom_target(
            new-class-tester
            COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/new_class_tester.cmake
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/new_class_tester.cmake
                    ${CMAKE_CURRENT_BINARY_DIR}/class_tester.tpl
                    ${_DSGT_BASE_DIR}/class_tester.tpl.in
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generate unit-test skeleton file"
          )
    endif()
endfunction()

# X-Chewy-RepoBase: https://raw.githubusercontent.com/mutanabbi/chewy-cmake-rep/master/
# X-Chewy-Path: DefineSkeletonGenerationTargetsIfPossible.cmake
# X-Chewy-Version: 6.13
# X-Chewy-Description: Add targets to generate class header/implementation and unit-tests skeleton files
# X-Chewy-AddonFile: TestCMakeLists.txt.in
# X-Chewy-AddonFile: class.tpl.in
# X-Chewy-AddonFile: class_tester.tpl.in
# X-Chewy-AddonFile: new_class_tester_post.cmake.in
# X-Chewy-AddonFile: render_template.cmake.in
