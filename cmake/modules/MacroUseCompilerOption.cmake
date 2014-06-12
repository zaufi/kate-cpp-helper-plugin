# - Append an option to compiler's flags if supported
#
# Check if given flag supported by a compiler and append to default
# compiler options if it does, or issue an error if the flag marked as REQUIRED.
#
# Synopsis:
#       use_compiler_option(
#           OPTION -<option-flag>
#           [REQUIRED]
#           LANGUAGE <C|CXX>
#           ALSO_PASS_TO_LINKER
#           OUTPUT <variable-name>
#         )
#
# Where,
#   OPTION -- compiler flag to check
#   REQUIRED -- issue an error if given option is not supported
#   LANGUAGE -- specify language (C or CXX supported nowadays)
#   OUTPUT -- variable name to set w/ a result
#

#=============================================================================
# Copyright 2010 by Alex Turbov <i.zaufi@gmail.com>
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

macro(use_compiler_option)
    set(options REQUIRED ALSO_PASS_TO_LINKER)
    set(one_value_args OPTION OUTPUT)
    set(multi_value_args LANGUAGE)
    cmake_parse_arguments(use_compiler_option "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT use_compiler_option_LANGUAGE)
        message(FATAL_ERROR "Required LANGUAGE parameter is not provided to `use_compiler_option`")
    endif()
    if(NOT use_compiler_option_OUTPUT)
        message(FATAL_ERROR "Required OUTPUT parameter is not provided to `use_compiler_option`")
    endif()

    if(use_compiler_option_OPTION)
        # Iterate over all languages
        foreach(lang ${use_compiler_option_LANGUAGE})
            # ATTENTION CheckSourceCompiles (used inside of `check_c_compiler_flag') do not pass
            # tested option to the likner, so some options, while test, can fail!
            # `-fsanitize=' is one of them...
            # So to make it work, lets add an option to test to the
            # required "libraries" list if ALSO_PASS_TO_LINKER option is given
            if(${use_compiler_option_ALSO_PASS_TO_LINKER})
                set(use_compiler_option_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
                set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES} ${use_compiler_option_OPTION}")
            endif()
            # Make sure current option is supported by current compiler
            if(${lang} STREQUAL "C")
                check_c_compiler_flag(${use_compiler_option_OPTION} ${use_compiler_option_OUTPUT})
            elseif(${lang} STREQUAL "CXX")
                check_cxx_compiler_flag(${use_compiler_option_OPTION} ${use_compiler_option_OUTPUT})
            else()
                message(FATAL_ERROR "Unsupported language ${lang}")
            endif()
            if(${use_compiler_option_ALSO_PASS_TO_LINKER})
                set(CMAKE_REQUIRED_LIBRARIES "${use_compiler_option_CMAKE_REQUIRED_LIBRARIES}")
            endif()
            if(${use_compiler_option_OUTPUT})
                set(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} ${use_compiler_option_OPTION}")
            elseif(use_compiler_option_REQUIRED)
                message(
                    FATAL_ERROR
                    "The ${lang} compiler doesn't support '${use_compiler_option_OPTION}' option, "
                    "which is required to build this package"
                  )
            endif()
        endforeach()
    else()
        message(FATAL_ERROR "No flag(s) given to `use_compiler_option`")
    endif()
endmacro(use_compiler_option)

# X-Chewy-RepoBase: https://raw.githubusercontent.com/mutanabbi/chewy-cmake-rep/master/
# X-Chewy-Path: MacroUseCompilerOption.cmake
# X-Chewy-Version: 1.4
# X-Chewy-Description: Append an option to compiler's flags if supported
