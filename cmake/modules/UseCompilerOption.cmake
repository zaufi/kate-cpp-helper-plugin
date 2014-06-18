# - Append an option to compiler's flags if supported
#
# Check if given flag supported by a compiler and append to default
# compiler options if it does, or issue an error if the flag marked as REQUIRED.
#
# NOTE Some options (like -pthread or -fsanitize=...) must be added to the
# linker options as well, so ALSO_PASS_TO_LINKER should be used.
#
# Synopsis:
#       use_compiler_option(
#           option
#           OUTPUT <variable-name>
#           [LANGUAGE <lang>]
#           [ALSO_PASS_TO_LINKER]
#           [REQUIRED]
#         )
#
# Where,
#   option      -- compiler/linker flag to check
#   LANGUAGE    -- specify language to check option for. C and/or CXX supported.
#                  (default: all enabled project languages)
#   REQUIRED    -- issue an error if given option is not supported
#   OUTPUT      -- variable name to set w/ a result
#

#=============================================================================
# Copyright 2010-2014 by Alex Turbov <i.zaufi@gmail.com>
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


include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CMakeParseArguments)

function(use_compiler_option OPTION)
    set(options REQUIRED ALSO_PASS_TO_LINKER)
    set(one_value_args OUTPUT)
    set(multi_value_args LANGUAGE)
    cmake_parse_arguments(use_compiler_option "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT use_compiler_option_OUTPUT)
        message(FATAL_ERROR "Required OUTPUT parameter is not provided to `use_compiler_option`")
    endif()

    if(NOT use_compiler_option_LANGUAGE)
        get_property(
            use_compiler_option_LANGUAGE
            GLOBAL PROPERTY ENABLED_LANGUAGES
          )
        if(NOT use_compiler_option_LANGUAGE)
            message(FATAL_ERROR "Required LANGUAGE parameter is not provided to `use_compiler_option`")
        endif()
    endif()

    if(OPTION)
        # Iterate over all languages
        foreach(lang ${use_compiler_option_LANGUAGE})
            set(_option "${OPTION}")
            set(_add_to)

            # Check if option looks like a linker option which is useless for compiler itself
            if(_option MATCHES "-Wl,")
                set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES} ${_option}")
                set(_option "")
                list(APPEND _add_to CMAKE_EXE_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS)
            elseif(use_compiler_option_ALSO_PASS_TO_LINKER)
                set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES} ${_option}")
                set(_option "")
                list(APPEND _add_to CMAKE_${lang}_FLAGS CMAKE_EXE_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS)
            else()
                list(APPEND _add_to CMAKE_${lang}_FLAGS)
            endif()

            # Make sure current option is supported by current compiler
            if(lang STREQUAL "C")
                check_c_compiler_flag("${_option}" ${use_compiler_option_OUTPUT})
            elseif(lang STREQUAL "CXX")
                check_cxx_compiler_flag("${_option}" ${use_compiler_option_OUTPUT})
            else()
                message(FATAL_ERROR "Unsupported language ${lang}")
            endif()

            # Test the result
            if(${${use_compiler_option_OUTPUT}})
                foreach(_opt ${_add_to})
                    string(FIND "${${_opt}}" "${OPTION}" _found)
                    if(_found STREQUAL "-1")
                        set(${_opt} "${${_opt}} ${OPTION}" PARENT_SCOPE)
                    endif()
                endforeach()
            elseif(use_compiler_option_REQUIRED)
                message(
                    FATAL_ERROR
                    "The ${lang} compiler doesn't support '${OPTION}' option, "
                    "which is required to build this package"
                  )
            endif()
        endforeach()
    else()
        message(FATAL_ERROR "No flag(s) given to `use_compiler_option`")
    endif()
endfunction(use_compiler_option)

# X-Chewy-RepoBase: https://raw.githubusercontent.com/mutanabbi/chewy-cmake-rep/master/
# X-Chewy-Path: UseCompilerOption.cmake
# X-Chewy-Version: 1.5
# X-Chewy-Description: Append an option to compiler's flags if supported
