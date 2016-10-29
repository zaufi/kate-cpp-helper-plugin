# - Getting value by a key from simple shell compatible
#   trivial config file
#
# Some data needed for build configuration may resides in a
# simple form of `key=value` wich is widely used to store
# configuration data for shell scripts.
#
#

#=============================================================================
# Copyright 2016 by Alex Turbov <i.zaufi@gmail.com>
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


function(get_value_from_config_file FILENAME)
    if(NOT UNIX)
        message(FATAL_ERROR "Sorry, not implemented for this OS yet")
    endif()

    if(FILENAME)
        set(_get_value_from_config_file_FILENAME "${FILENAME}")
    else()
        message(FATAL_ERROR "No filename has given in call to `get_value_from_config_file()`")
    endif()

    set(_options)
    set(_one_value_args KEY OUTPUT_VARIABLE)
    set(_multi_value_args)
    cmake_parse_arguments(_get_value_from_config_file "${_options}" "${_one_value_args}" "${_multi_value_args}" ${ARGN})

    if(NOT _get_value_from_config_file_KEY)
        message(FATAL_ERROR "No `KEY` has given in call to `get_value_from_config_file()`")
    endif()
    if(NOT _get_value_from_config_file_OUTPUT_VARIABLE)
        message(FATAL_ERROR "No `OUTPUT_VARIABLE` has given in call to `get_value_from_config_file()`")
    endif()

    execute_process(
        COMMAND /bin/sh -c ". ${_get_value_from_config_file_FILENAME} && echo \${${_get_value_from_config_file_KEY}}"
        OUTPUT_VARIABLE _result
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    set(${_get_value_from_config_file_OUTPUT_VARIABLE} "${_result}" PARENT_SCOPE)

endfunction()

# X-Chewy-RepoBase: https://raw.githubusercontent.com/mutanabbi/chewy-cmake-rep/master/
# X-Chewy-Path: GetKeyValueFromShellLikeConfig.cmake
# X-Chewy-Version: 1.0
# X-Chewy-Description: Getting values from trivial shell compatible config files
