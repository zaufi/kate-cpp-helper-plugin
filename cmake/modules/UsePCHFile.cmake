# - Function to render a header file that can be used as PCH
#
# `use_pch_file()` will add a target (named 'update-pch-header') to collect
# most used imported (i.e. not from this project) headers
# and generate a file w/ #include directives for all of them.
#
#   use_pch_file(PCH_FILE <file>
#       EXCLUDE_DIRS dir1 [... dirN]
#       EXCLUDE_HEADERS file1 [... fileN]
#     )
#
# Parameters are:
#
#   PCH_FILE        -- an output filename to be generated
#   EXCLUDE_DIRS    -- a list of directories to exclude from scan
#   EXCLUDE_HEADERS -- a list of header files to exclude from result PCH file
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

set(_USE_PHC_FILE_MODULE_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(use_pch_file)
    set(oneValueArgs PCH_FILE)
    set(multiValueArgs EXCLUDE_DIRS EXCLUDE_HEADERS)
    cmake_parse_arguments(use_pch_file "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (use_pch_file_EXCLUDE_DIRS)
        foreach(_dir ${use_pch_file_EXCLUDE_DIRS})
        set(use_pch_file_EXCLUDE_DIRS_REGEX "${use_pch_file_EXCLUDE_DIRS_REGEX}${_pipe}${_dir}")
        set(_pipe "|")
        endforeach()
        unset(_pipe)
        set(
            use_pch_file_FILTER_DIRS_COMMAND
            "COMMAND egrep -v \"(${use_pch_file_EXCLUDE_DIRS_REGEX})/\""
          )
    endif()

    if (use_pch_file_EXCLUDE_HEADERS)
        foreach(_dir ${use_pch_file_EXCLUDE_HEADERS})
        set(use_pch_file_EXCLUDE_HEADERS_REGEX "${use_pch_file_EXCLUDE_HEADERS_REGEX}${_pipe}${_dir}")
        set(_pipe "|")
        endforeach()
        unset(_pipe)
        set(
            use_pch_file_FILTER_FILES_COMMAND
            "COMMAND egrep -v \"(${use_pch_file_EXCLUDE_HEADERS_REGEX})\""
          )
    endif()

    # Render a script to produce a header file w/ most used external headers
    configure_file(
        ${_USE_PHC_FILE_MODULE_BASE_DIR}/PreparePCHHeader.cmake.in
        ${CMAKE_BINARY_DIR}/PreparePCHHeader.cmake
        @ONLY
      )

    add_custom_target(
        update-pch-header
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/PreparePCHHeader.cmake
        MAIN_DEPENDENCY ${CMAKE_BINARY_DIR}/PreparePCHHeader.cmake
        COMMENT "Updating ${use_pch_file_PCH_FILE}"
      )

endfunction()

# kate: hl cmake;
# X-Chewy-RepoBase: https://raw.github.com/mutanabbi/chewy-cmake-rep/master/
# X-Chewy-Path: UsePCHFile.cmake
# X-Chewy-Version: 2.7
# X-Chewy-Description: Add Precompiled Header Support
# X-Chewy-AddonFile: PreparePCHHeader.cmake.in
# X-Chewy-AddonFile: pch-template.h.in
