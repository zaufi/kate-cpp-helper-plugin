# - Get distribution code name
# Get a distribution codename according LSB spec or vendor specific files (for Linux)
# or use voluntary strings (for Windows).
#
# Set the following variables:
#   DISTRIB_ID          -- a distribution identifier (like Gentoo or Ubuntu & etc)
#   DISTRIB_ARCH        -- a distribution target machine
#   DISTRIB_CODENAME    -- a distribution code name (like "quantal" or "trusty" for Ubuntu)
#   DISTRIB_VERSION     -- a version string of the dictribution
#   DISTRIB_PKG_FMT     -- native package manager's format(s) suitable to use w/ CPACK_GENERATOR
#   DISTRIB_SRC_PKG_FMT -- native format to create tarballs
#   DISTRIB_FILE_PART   -- a string suitable to be a filename part to identify a target system
#   DISTRIB_PKG_VERSION -- if dictribution has package manager, this variable set to a string
#                          used by native packages as distribution identifier (like `1.el7.centos`)
#
# NOTE DISTRIB_PKG_FMT will not contain "generic" archive formats!
#

#=============================================================================
# Copyright 2012-2016 by Alex Turbov <i.zaufi@gmail.com>
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

include("${CMAKE_CURRENT_LIST_DIR}/GetKeyValueFromShellLikeConfig.cmake")

set(DEFAULT_DISTRIB_CODENAME "auto" CACHE STRING "Target distribution codename")
set(DEFAULT_DISTRIB_ID "auto" CACHE STRING "Target distribution")

#
# Form a filename part for the current distro
#
macro(_make_distrib_file_part)
    string(APPEND DISTRIB_FILE_PART "${DISTRIB_ID}")
    if(WIN32)
        string(APPEND DISTRIB_FILE_PART "${DISTRIB_ARCH}")
    else()
        if(DISTRIB_VERSION)
            string(REPLACE "." ";" DISTRIB_VERSION_LIST "${DISTRIB_VERSION}")
            list(LENGTH DISTRIB_VERSION_LIST DISTRIB_VERSION_LIST_LEN)
            if(NOT DISTRIB_VERSION_LIST_LEN LESS 1)
                list(GET DISTRIB_VERSION_LIST 0 DISTRIB_VERSION_MAJOR)
            endif()
            if(NOT DISTRIB_VERSION_LIST_LEN LESS 2)
                list(GET DISTRIB_VERSION_LIST 1 DISTRIB_VERSION_MINOR)
            endif()
            if(NOT DISTRIB_VERSION_LIST_LEN LESS 3)
                list(GET DISTRIB_VERSION_LIST 2 DISTRIB_VERSION_PATCH)
            endif()
            if(DISTRIB_VERSION_MAJOR)
                string(APPEND DISTRIB_FILE_PART "${DISTRIB_VERSION_MAJOR}")
            endif()
            if(DISTRIB_VERSION_MINOR)
                string(APPEND DISTRIB_FILE_PART ".${DISTRIB_VERSION_MINOR}")
            endif()
        endif()
        if(DISTRIB_ARCH)
            string(APPEND DISTRIB_FILE_PART "-${DISTRIB_ARCH}")
        endif()
    endif()
    # Make a string suitable to be a filename part to identify a target system
    string(TOLOWER "${DISTRIB_FILE_PART}" DISTRIB_FILE_PART)
endmacro()

macro(_try_check_centos _release_file)
    set(DISTRIB_ID "CentOS")
    file(STRINGS ${_release_file} _release_string)
    # NOTE CentOS 6.0 has a word "Linux" in release string
    string(REGEX REPLACE "CentOS (Linux )?release ([0-9\\.]+) .*" "\\2" DISTRIB_VERSION "${_release_string}")
    # Set native packages format
    set(DISTRIB_PKG_FMT "RPM")
    set(DISTRIB_SRC_PKG_FMT "TBZ2")
    set(DISTRIB_HAS_PACKAGE_MANAGER TRUE)
    # TODO Get more details
endmacro()

macro(_try_check_redhat _release_string)
    if(_release_string MATCHES "Red Hat Enterprise Linux Server")
        set(DISTRIB_ID "RHEL")
        string(
            REGEX REPLACE
                "Red Hat Enterprise Linux Server release ([0-9\\.]+) .*" "\\1"
            DISTRIB_VERSION
            "${_release_string}"
          )
        string(
            REGEX REPLACE
                "Red Hat Enterprise Linux Server release [0-9\\.]+ \((.*)\)" "\\1"
            DISTRIB_CODENAME
            "${_release_string}"
          )
    # Set native packages format
    set(DISTRIB_PKG_FMT "RPM")
    set(DISTRIB_SRC_PKG_FMT "TBZ2")
    set(DISTRIB_HAS_PACKAGE_MANAGER TRUE)
    endif()
endmacro()

function(_debug msg)
    if(GDI_DEBUG)
        message(STATUS "[GDI] ${msg}")
    endif()
endfunction()

#
# Ok, lets collect come info about this distro...
#
if(NOT DISTRIB_ID)
    if(NOT WIN32)
        _debug("We r not in Windows! Trying `uname`")
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
            _debug("`uname -m` returns ${DISTRIB_ARCH}")
        endif()
    endif()

    # Trying Windows
    if(WIN32)
        _debug("Windows detected. Checking `void*` size")
        set(DISTRIB_ID "Win")
        set(DISTRIB_PKG_FMT "WIX")
        set(DISTRIB_SRC_PKG_FMT "ZIP")
        set(DISTRIB_VERSION "${CMAKE_SYSTEM_VERSION}")

        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            _debug("  `void*` size is 8")
            set(DISTRIB_ARCH "64")
            list(APPEND DISTRIB_PKG_FMT "NSIS64")
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
            _debug("  `void*` size is 4")
            set(DISTRIB_ARCH "32")
            list(APPEND DISTRIB_PKG_FMT "NSIS")
        elseif(NOT CMAKE_SIZEOF_VOID_P)
            _debug("  `void*` size is undefined")
            set(DISTRIB_ARCH "-noarch")
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
        elseif(_release_string MATCHES "Red Hat")
            _try_check_redhat(${_release_string})
        endif()
        # TODO Detect a real RH releases

    elseif(EXISTS /etc/gentoo-release)

        set(DISTRIB_ID "Gentoo")
        set(DISTRIB_PKG_FMT "TBZ2")
        set(DISTRIB_SRC_PKG_FMT "TBZ2")
        # Try to tune DISTRIB_ARCH
        if(DISTRIB_ARCH STREQUAL "x86_64")
            # 64-bit packets usualy named amd64 here...
            set(DISTRIB_ARCH "amd64")
        endif()
        # TODO Get more details

    # Trying LSB conformant distros like Ubuntu, RHEL or CentOS w/
    # corresponding package installed. What else?
    elseif(EXISTS /usr/bin/lsb_release)

        # Get DISTRIB_ID
        execute_process(
            COMMAND /usr/bin/lsb_release -i
            OUTPUT_VARIABLE DISTRIB_ID
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
          )
        string(REGEX REPLACE ".+:[\t ]+([A-Za-z]+).*" "\\1" DISTRIB_ID "${DISTRIB_ID}")
        # Get DISTRIB_CODENAME
        execute_process(
            COMMAND /usr/bin/lsb_release -c
            OUTPUT_VARIABLE DISTRIB_CODENAME
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
          )
        string(REGEX REPLACE ".+:[\t ]+(.+)" "\\1" DISTRIB_CODENAME "${DISTRIB_CODENAME}")
        # Get DISTRIB_VERSION
        execute_process(
            COMMAND /usr/bin/lsb_release -r
            OUTPUT_VARIABLE DISTRIB_VERSION
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
          )
        string(REGEX REPLACE ".+:[\t ]+(.+).*" "\\1" DISTRIB_VERSION "${DISTRIB_VERSION}")
        # Set native packages format
        set(DISTRIB_SRC_PKG_FMT "TBZ2")
        if(DISTRIB_ID STREQUAL "CentOS" OR DISTRIB_ID STREQUAL "RedHat")
            set(DISTRIB_PKG_FMT "RPM")
            set(DISTRIB_HAS_PACKAGE_MANAGER TRUE)
        elseif(DISTRIB_ID STREQUAL "Ubuntu")
            set(DISTRIB_PKG_FMT "DEB")
            set(DISTRIB_HAS_PACKAGE_MANAGER TRUE)
            # Try tune DISTRIB_ARCH
            if(DISTRIB_ARCH STREQUAL "x86_64")
                # 64-bit packets usualy named amd64 here...
                set(DISTRIB_ARCH "amd64")
            endif()
        else()
            # TODO Anything else?
            _debug("LSB compliant distro detected, but not fully recognized")
        endif()

    # Trying LSB conformant distros but w/o corresponding package installed.
    elseif(EXISTS /etc/lsb-release)
        get_value_from_config_file(/etc/lsb-release KEY "DISTRIB_ID" OUTPUT_VARIABLE DISTRIB_ID)
        get_value_from_config_file(/etc/lsb-release KEY "DISTRIB_RELEASE" OUTPUT_VARIABLE DISTRIB_VERSION)
        get_value_from_config_file(/etc/lsb-release KEY "DISTRIB_CODENAME" OUTPUT_VARIABLE DISTRIB_CODENAME)
        if(DISTRIB_ID STREQUAL "Ubuntu")
            set(DISTRIB_PKG_FMT "DEB")
            set(DISTRIB_HAS_PACKAGE_MANAGER TRUE)
            # Try tune DISTRIB_ARCH
            if(DISTRIB_ARCH STREQUAL "x86_64")
                # 64-bit packets usualy named amd64 here...
                set(DISTRIB_ARCH "amd64")
            endif()
        else()
            # TODO What other distros??
            _debug("/etc/lsb-release exists, but not fully recognized")
        endif()

    else()
        # Try generic way
        if(UNAME_EXECUTABLE)
            # Try to get kernel name
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
        # Try to tune it
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
    endif()

    _make_distrib_file_part()

    if(DISTRIB_ID)
        set(_distrib_info_line "${DISTRIB_ID}")

        if(DISTRIB_VERSION)
            set(_distrib_info_line "${_distrib_info_line} ${DISTRIB_VERSION}")
        endif()

        if(DISTRIB_FILE_PART)
            set(_distrib_info_line "${_distrib_info_line} [${DISTRIB_FILE_PART}]")
        endif()

        message(STATUS "Target distribution: ${_distrib_info_line}")

        unset(_distrib_info_line)
    else()
        message(STATUS "Target distribution is unknown")
    endif()
endif()

# X-Chewy-RepoBase: https://raw.githubusercontent.com/mutanabbi/chewy-cmake-rep/master/
# X-Chewy-Path: GetDistribInfo.cmake
# X-Chewy-Version: 3.0
# X-Chewy-Description: Get a distribution codename
# X-Chewy-AddonFile: GetKeyValueFromShellLikeConfig.cmake
