# Copyright 2012 by Alex Turbov <i.zaufi@gmail.com>
#
# Setup build type. If no CMAKE_BUILD_TYPE provided to `cmake`
# try to guess it. The end-user, who get a tarball, likely wants
# to build a release, but developers (who has VCS dirs on top of a
# source tree), more likely want to build a debug version...
#

# If CMAKE_BUILD_TYPE is not set, check for VCS files
if (NOT CMAKE_BUILD_TYPE)
    if (EXISTS ${CMAKE_SOURCE_DIR}/.git OR EXISTS ${CMAKE_SOURCE_DIR}/.hg OR EXISTS ${CMAKE_SOURCE_DIR}/.svn)
        set(CMAKE_BUILD_TYPE "Debug")
    else()
        set(CMAKE_BUILD_TYPE "Release")
    endif()
endif()
message(STATUS "Configuration type choosen: ${CMAKE_BUILD_TYPE}")

# kate: hl cmake;
# X-Chewy-RepoBase: https://raw.github.com/mutanabbi/chewy-cmake-rep/master/
# X-Chewy-Path: GuessBuildType.cmake
# X-Chewy-Version: 1.2
# X-Chewy-Description: Helper to install Python scripts
