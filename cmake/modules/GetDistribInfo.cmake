# - Get distribution code name
# Get a distribution codename according LSB spec or vendor specific files (for Linux)
# or use voluntary strings (for Windows).
#
# Set the following variables:
#   DISTRIB_ID          -- a distribution identifier (like Gentoo or Ubuntu)
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

set(DEFAULT_DISTRIB_CODENAME "unknown" CACHE STRING "Target distribution codename")

if(NOT DISTRIB_CODENAME)
    # Trying Windows
    if(WIN32)

        set(DISTRIB_PKG_FMT "NSIS;WIX")
        if(MSVC)
            if(CMAKE_CL_64)
                set(DISTRIB_ID "Win64")
            else()
                set(DISTRIB_ID "Win32")
            endif()
        endif()
        # Make a string suitable to be a filename part to identify a target system
        string(TOLOWER "${DISTRIB_ID}" DISTRIB_FILE_PART)

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
        # Make a string suitable to be a filename part to identify a target system
        string(TOLOWER "${DISTRIB_ID}-${DISTRIB_VERSION}" DISTRIB_FILE_PART)

    # Trying CentOS distros
    elseif(EXISTS /etc/centos-release)
        # ATTENTION CentOS has a symlink /etc/redhat-release -> /etc/centos-release,
        # so it mut be handled before!
        # NOTE /etc/centos-release
        set(DISTRIB_ID "CentOS")
        set(DISTRIB_FILE_PART "centos")
        file(STRINGS /etc/centos-release DISTRIB_VERSION)
        string(REGEX REPLACE "CentOS release ([0-9.]+) .*" "\\1" DISTRIB_VERSION "${DISTRIB_VERSION}")
        # Set native packages format
        set(DISTRIB_PKG_FMT "RPM")
        # TODO Get more details

    # Trying RedHat distros
    elseif(EXISTS /etc/redhat-release)

        file(STRINGS /etc/redhat-release DISTRIB_CODENAME)
        string(REGEX REPLACE ".*\((.*)\)" "\\1" DISTRIB_CODENAME "${DISTRIB_CODENAME}")
        string(TOLOWER "${DISTRIB_CODENAME}" DISTRIB_CODENAME)
        # Set native packages format
        set(DISTRIB_PKG_FMT "RPM")
        # TODO Get more details

    elseif(EXISTS /etc/gentoo-release)

        set(DISTRIB_ID "gentoo")
        set(DISTRIB_PKG_FMT "")
        # Make a string suitable to be a filename part to identify a target system
        set(DISTRIB_FILE_PART "gentoo")
        # TODO Get more details

    else()
        set(DISTRIB_CODENAME "${DEFAULT_DISTRIB_CODENAME}" CACHE INTERNAL "Target distribution codename")
    endif()
    message(STATUS "Target distribution: ${DISTRIB_ID} ${DISTRIB_VERSION} ${DISTRIB_CODENAME}")
endif()

# X-Chewy-RepoBase: https://raw.githubusercontent.com/mutanabbi/chewy-cmake-rep/master/
# X-Chewy-Path: GetDistribInfo.cmake
# X-Chewy-Version: 2.4
# X-Chewy-Description: Get a distribution codename
