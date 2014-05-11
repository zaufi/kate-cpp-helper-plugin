# - Guess appropriate value for CMAKE_BUILD_TYPE if latter is not set
#
# If no CMAKE_BUILD_TYPE provided to `cmake` try to guess it.
# The end-user, who get a tarball, likely wants
# to build a release, but developers (who has VCS dirs on top of a
# source tree), likely wants to build a debug version...
#

#=============================================================================
# Copyright 2012 by Alex Turbov <i.zaufi@gmail.com>
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

# If CMAKE_BUILD_TYPE is not set, check for VCS files
if (NOT CMAKE_BUILD_TYPE)
    if (EXISTS ${CMAKE_SOURCE_DIR}/.git OR EXISTS ${CMAKE_SOURCE_DIR}/.hg OR EXISTS ${CMAKE_SOURCE_DIR}/.svn)
        set(CMAKE_BUILD_TYPE "Debug")
    else()
        set(CMAKE_BUILD_TYPE "Release")
    endif()
endif()
message(STATUS "Configuration type choosen: ${CMAKE_BUILD_TYPE}")

# X-Chewy-RepoBase: https://raw.githubusercontent.com/mutanabbi/chewy-cmake-rep/master/
# X-Chewy-Path: GuessBuildType.cmake
# X-Chewy-Version: 1.5
# X-Chewy-Description: Guess build type if not specified explicitly
