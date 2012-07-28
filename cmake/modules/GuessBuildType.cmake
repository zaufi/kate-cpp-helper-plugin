# Copyright 2012 by Alex Turbov <i.zaufi@gmail.com>
#
# Setup build type

# If CMAKE_BUILD_TYPE is not set, check for VCS files
if (NOT CMAKE_BUILD_TYPE)
    if (EXISTS ${CMAKE_SOURCE_DIR}/.git)
        set(CMAKE_BUILD_TYPE "Debug")
    else()
        set(CMAKE_BUILD_TYPE "Release")
    endif()
endif()
message(STATUS "Configuration type choosen: ${CMAKE_BUILD_TYPE}")
# kate: hl cmake;
