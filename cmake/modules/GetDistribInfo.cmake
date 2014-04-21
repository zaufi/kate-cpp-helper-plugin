# Copyright 2012-2014 by Alex Turbov <i.zaufi@gmail.com>
#
# Function (macro actually :) to get a distribution codename according LSB spec
#

set(DEFAULT_DISTRIB_CODENAME "unknown" CACHE STRING "Target distribution codename according LSB")

if(NOT DISTRIB_CODENAME)
    if(EXISTS /etc/lsb-release)
        file(STRINGS /etc/lsb-release distrib_codename_line REGEX "DISTRIB_CODENAME=")
        string(REGEX REPLACE "DISTRIB_CODENAME=\"?(.*)\"?" "\\1" DISTRIB_CODENAME "${distrib_codename_line}")
        set(DISTRIB_PKG_FMT "DEB")
    elseif(EXISTS /etc/redhat-release)
        file(STRINGS /etc/redhat-release distrib_codename_line)
        string(REGEX REPLACE ".*\((.*)\)" "\\1" DISTRIB_CODENAME "${distrib_codename_line}")
        string(TOLOWER DISTRIB_CODENAME "${DISTRIB_CODENAME}")
        set(DISTRIB_PKG_FMT "RPM")
    elseif(EXISTS /etc/gentoo-release)
        set(DISTRIB_CODENAME "gentoo")
        set(DISTRIB_PKG_FMT "TBZ2")
    else()
        set(DISTRIB_CODENAME "${DEFAULT_DISTRIB_CODENAME}" CACHE INTERNAL "Target distribution codename")
    endif()
    message(STATUS "Target distribution codename: ${DISTRIB_CODENAME}")
endif()

# X-Chewy-RepoBase: https://raw.github.com/mutanabbi/chewy-cmake-rep/master/
# X-Chewy-Path: GetDistribInfo.cmake
# X-Chewy-Version: 2.0
# X-Chewy-Description: Get a distribution codename according LSB spec or vendor specific files
