# - Define a target to open a file/URI in the user's preferred application
#
# It uses `xdg-open` for Linux platform to open HTML docs or PDF documents...
# or anything else that possible to open w/ that tool.
#
# Usage:
#   add_open_target(
#       target-name
#       file-path
#       [DEPENDS dep1 ... depN]
#       [COMMENT text]
#     )
#
# Open a file (or URI) given as `file-path` using target named `target-name`.
# Dependencies actually can be files and/or other targets.
# Example:
#
#   add_open_target(
#       show-api-docs
#       ${CMAKE_BINARY_DIR}/docs/html/index.html
#       DEPENDS doxygen
#       COMMENT "Show API documentation"
#     )
#
# Will open given `index.html` in a default browser. For Makefiles generator
# it runs w/ `make show-api-docs` command. Also it depends on another target
# named `doxygen`
#

#=============================================================================
# Copyright 2014 by Alex Turbov <i.zaufi@gmail.com>
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

if(NOT WIN32 AND NOT XDG_OPEN_EXECUTABLE)
    find_program(
        XDG_OPEN_EXECUTABLE
        NAMES xdg-open
        DOC "opens a file or URL in the user's preferred application"
      )
    if(XDG_OPEN_EXECUTABLE)
        message(STATUS "Found xdg-open: ${XDG_OPEN_EXECUTABLE}")
    endif()
endif()

function(add_open_target target file)
    if(NOT XDG_OPEN_EXECUTABLE)
        return()
    endif()

    set(_options)
    set(_one_value_args COMMENT)
    set(_multi_value_args DEPENDS)
    cmake_parse_arguments(add_open_target "${_options}" "${_one_value_args}" "${_multi_value_args}" ${ARGN})

    if(NOT add_open_target_COMMENT)
        set(add_open_target_COMMENT "Opening ${file}...")
    endif()

    # Form lists of target and file dependencies
    set(_dep_targets)
    set(_dep_files)
    foreach(_d ${add_open_target_DEPENDS})
        # Check if current dependency is a target
        if(TARGET ${_d})
            if(_dep_targets)
                set(_dep_targets "${_dep_targets} ${_d}")
            else()
                set(_dep_targets "${_d}")
            endif()
        else()
            if(_dep_files)
                set(_dep_files "${_dep_files} ${_d}")
            else()
                set(_dep_files "${_d}")
            endif()
        endif()
    endforeach()

    if(_dep_files)
        add_custom_target(
            ${target}
            COMMAND ${XDG_OPEN_EXECUTABLE} ${file}
            DEPENDS ${_dep_files}
            COMMENT ${add_open_target_COMMENT}
            VERBATIM
        )
    else()
        add_custom_target(
            ${target}
            COMMAND ${XDG_OPEN_EXECUTABLE} ${file}
            COMMENT ${add_open_target_COMMENT}
            VERBATIM
          )
    endif()

    if(_dep_targets)
        add_dependencies(${target} ${_dep_targets})
    endif()
endfunction()

# X-Chewy-RepoBase: https://raw.githubusercontent.com/mutanabbi/chewy-cmake-rep/master/
# X-Chewy-Path: AddOpenTarget.cmake
# X-Chewy-Version: 1.2
# X-Chewy-Description: Define a target to open a file/URI in the user's preferred application
