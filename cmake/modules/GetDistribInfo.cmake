# - Get distribution code name
# Get a distribution codename according LSB spec or vendor specific files (for Linux)
# or use voluntary strings (for Windows).
#
# Set the following variables:
#   DISTRIB_ID          -- a distribution identifier (like Gentoo or Ubuntu)
#   DISTRIB_ARCH        -- a distribution target machine
#   DISTRIB_CODENAME    -- a distribution code name (like "quantal" or "trusty" for Ubuntu)
#   DISTRIB_VERSION     -- a version string of the dictribution
#   DISTRIB_PKG_FMT     -- native package manager's format suitable to use w/ CPACK_GENERATOR
#   DISTRIB_FILE_PART   -- a string suitable to be a filename part to identify a target system
#
# NOTE DISTRIB_PKG_FMT will not contain "generic" archive formats!
# TODO DISTRIB_CODENAME, DISTRIB_VERSION is not available for Windows
#

#=============================================================================
# Copyright 2012-2014 by Alex Turbov <i.zaufi@gmail.com>
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

set(DEFAULT_DISTRIB_CODENAME "auto" CACHE STRING "Target distribution codename")
set(DEFAULT_DISTRIB_ID "auto" CACHE STRING "Target distribution")

#
# ATTENTION Macro arguments are not variables!
# So they can't be checked just w/ `if(PARAM)`!
#
macro(_clue_file_part PART DELIMITER STATE OUTPUT)
    if(NOT STATE STREQUAL "")
        if(NOT PART STREQUAL "")
            set(${OUTPUT} "${STATE}${DELIMITER}${PART}")
        else()
            set(${OUTPUT} "${STATE}")
        endif()
    elseif(NOT PART STREQUAL "")
        set(${OUTPUT} "${PART}")
    endif()
endmacro()

#
# Form a filename part for the current distro
#
macro(_make_distrib_file_part)
    _clue_file_part("${DISTRIB_ID}" "" "" DISTRIB_FILE_PART)
    if(WIN32)
        _clue_file_part("${DISTRIB_ARCH}" "" "${DISTRIB_FILE_PART}" DISTRIB_FILE_PART)
    else()
        _clue_file_part("${DISTRIB_VERSION}" "" "${DISTRIB_FILE_PART}" DISTRIB_FILE_PART)
        _clue_file_part("${DISTRIB_ARCH}" "-" "${DISTRIB_FILE_PART}" DISTRIB_FILE_PART)
    endif()
    # Make a string suitable to be a filename part to identify a target system
    string(TOLOWER "${DISTRIB_FILE_PART}" DISTRIB_FILE_PART)
endmacro()

macro(_try_check_centos _release_file)
    set(DISTRIB_ID "CentOS")
    file(STRINGS ${_release_file} _release_string)
    # NOTE CentOS 6.0 has a word "Linux" in release string
    string(REGEX REPLACE "CentOS release ([0-9\\.]+) .*" "\\1" DISTRIB_VERSION "${_release_string}")
    # Set native packages format
    set(DISTRIB_PKG_FMT "RPM")
    # TODO Get more details
endmacro()

#
# Ok, lets collect come info about this distro...
#
if(NOT DISTRIB_CODENAME)
    if(NOT WIN32)
        find_program(
            UNAME_EXECUTABLE
            NAMES uname
            DOC "Print certain system information"
          )
        if(UNAME_EXECUTABLE)
            execute_process(
                COMMAND "${UNAME_EXECUTABLE}" -m
                OUTPUT_VARIABLE DISTRIB_ARCH
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
              )
            # NOTE DISTRIB_ARCH can be overriden (tuned) later
        endif()
    endif()

    # Trying Windows
    if(WIN32)

        set(DISTRIB_PKG_FMT "WIX")
        if(MSVC)
            set(DISTRIB_ID "Win")
            if(CMAKE_CL_64)
                set(DISTRIB_ARCH "64")
                list(APPEND DISTRIB_PKG_FMT "NSIS64")
            else()
                set(DISTRIB_ARCH "32")
                list(APPEND DISTRIB_PKG_FMT "NSIS")
            endif()
        endif()

    # Trying LSB conformant distros (like Ubuntu. What else?)
    elseif(EXISTS /etc/lsb-release)

        # Get DISTRIB_ID
        file(STRINGS /etc/lsb-release DISTRIB_ID REGEX "DISTRIB_ID=")
        string(REGEX REPLACE "DISTRIB_ID=\"?(.*)\"?" "\\1" DISTRIB_ID "${DISTRIB_ID}")
        # Get DISTRIB_CODENAME
        file(STRINGS /etc/lsb-release DISTRIB_CODENAME REGEX "DISTRIB_CODENAME=")
        string(REGEX REPLACE "DISTRIB_CODENAME=\"?(.*)\"?" "\\1" DISTRIB_CODENAME "${DISTRIB_CODENAME}")
        # Get DISTRIB_VERSION
        file(STRINGS /etc/lsb-release DISTRIB_VERSION REGEX "DISTRIB_RELEASE=")
        string(REGEX REPLACE "DISTRIB_RELEASE=\"?(.*)\"?" "\\1" DISTRIB_VERSION "${DISTRIB_VERSION}")
        # Set native packages format
        set(DISTRIB_PKG_FMT "DEB")
        # Try tune DISTRIB_ARCH
        if(DISTRIB_ARCH STREQUAL "x86_64")
            # 64-bit packets usualy named amd64 here...
            set(DISTRIB_ARCH "amd64")
        endif()

    # Trying CentOS distros
    elseif(EXISTS /etc/centos-release)
        # ATTENTION CentOS has a symlink /etc/redhat-release -> /etc/centos-release,
        # so it mut be handled before!
        # NOTE /etc/centos-release
        _try_check_centos(/etc/centos-release)

    # Trying RedHat distros
    elseif(EXISTS /etc/redhat-release)

        file(STRINGS /etc/redhat-release _release_string)
        if(_release_string MATCHES "CentOS")
            _try_check_centos(/etc/redhat-release)
        endif()
        # TODO Detect a real RH releases

    elseif(EXISTS /etc/gentoo-release)

        set(DISTRIB_ID "gentoo")
        set(DISTRIB_PKG_FMT "")
        # Try tune DISTRIB_ARCH
        if(DISTRIB_ARCH STREQUAL "x86_64")
            # 64-bit packets usualy named amd64 here...
            set(DISTRIB_ARCH "amd64")
        endif()
        # TODO Get more details

    else()
        # Try generic way
        if(UNAME_EXECUTABLE)
            # Try get kernel name
            execute_process(
                COMMAND "${UNAME_EXECUTABLE}" -o
                OUTPUT_VARIABLE DISTRIB_ID
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
              )
            execute_process(
                COMMAND "${UNAME_EXECUTABLE}" -r
                OUTPUT_VARIABLE DISTRIB_VERSION
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
              )
        endif()
        # Try tune it
        if(DISTRIB_ID STREQUAL "Solaris")
            if(EXISTS /etc/release)
                file(STRINGS /etc/release DISTRIB_CODENAME REGEX "SmartOS")
                if(NOT DISTRIB_CODENAME STREQUAL "")
                    set(DISTRIB_ID "SmartOS")
                    set(DISTRIB_CODENAME "")
                    # NOTE According docs, SmartOS has no version.
                    # It has a timestamp of a base image instead.
                    set(DISTRIB_VERSION "")
                endif()
            endif()
        endif()
    else()
        set(DISTRIB_ID "${DEFAULT_DISTRIB_ID}" CACHE INTERNAL "Target distribution")
        set(DISTRIB_CODENAME "${DEFAULT_DISTRIB_CODENAME}" CACHE INTERNAL "Target distribution codename")
    endif()

    _make_distrib_file_part()

    message(STATUS "Target distribution: ${DISTRIB_ID} ${DISTRIB_VERSION} ${DISTRIB_CODENAME} [${DISTRIB_FILE_PART}]")
endif()

# X-Chewy-RepoBase: https://raw.githubusercontent.com/mutanabbi/chewy-cmake-rep/master/
# X-Chewy-Path: GetDistribInfo.cmake
# X-Chewy-Version: 2.10
# X-Chewy-Description: Get a distribution codename
