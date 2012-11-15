#
# Copyright 2012 by Alex Turbov <i.zaufi@gmail.com>
#
# kate: hl cmake;

#---------------------------------------------------------
# Section: Init cmake
#---------------------------------------------------------
project(KateIncludeHelperPlugin C CXX)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

#---------------------------------------------------------
# Section: Include aux cmake functions
#---------------------------------------------------------
include(GuessBuildType)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(CTest)
endif()

#---------------------------------------------------------
# Section: Find used stuff
#---------------------------------------------------------

# Find KDE
find_package(KDE4 REQUIRED)
include(KDE4Defaults)

# find boost
# NOTE I see no way to make unit tests optional for release mode
# BUILD_TESTING will always turned ON... so caller have to disable
# it explicitly if required.
if (BUILD_TESTING)
    find_package(Boost 1.42 COMPONENTS unit_test_framework)
    if (Boost_UNIT_TEST_FRAMEWORK_LIBRARY MATCHES NOTFOUND)
        message(STATUS "Unit tests disabled")
        set(BUILD_TESTING OFF)
    else()
        include(AddBoostTests)
    endif()
else()
    message(STATUS "Unit tests disabled")
endif()

#---------------------------------------------------------
# Section: Configure building process
#---------------------------------------------------------
# NOTE Order is important!
include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR})
include_directories(${KDE4_INCLUDES} ${QT_INCLUDES} ${Boost_INCLUDE_DIRS})

# Tell to boost::result_of to use decltype to decay return type of callable.
# NOTE This would enable to use C++11 labmda expressions w/ boost::rage adaptors
#add_definitions(-DBOOST_RESULT_OF_USE_DECLTYPE)
# Add Qt/KDE defines
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
# Use Kate plugins debug area as default
# add_definitions(-DDEBUG_AREA=13040)

# Set generic compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

#---------------------------------------------------------
# Section: Define some helpful targets (using included)
#          functions.
# ALERT It should be at the bottom!
#       To make sure all vars r defined!
#---------------------------------------------------------

#include(DefineDoxyDocsTargetIfPossible)
set(NAMING_STYLE Camel)
include(DefineSkeletonGenerationTargetsIfPossible)

#---------------------------------------------------------
# Section: Dive into subdirs after main configuration
#---------------------------------------------------------
add_subdirectory(src)

#---------------------------------------------------------
# Section: Configure package options
#---------------------------------------------------------
set(CPACK_GENERATOR TBZ2)
set(CPACK_SOURCE_GENERATOR TBZ2)
set(
    CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${VERSION_STRING}"
    CACHE INTERNAL "tarball basename"
  )
# The following components are regex's to match anywhere (unless anchored)
# in absolute path + filename to find files or directories to be excluded
# from source tarball.
set(CPACK_SOURCE_IGNORE_FILES
    "\\\\.gitignore$";
    "\\\\.kate-swp$";
    "\\\\.tar\\\\.gz$";
    "^.*/\\\\.git.*$"
  )

# Include CPack support
include(CPack)
