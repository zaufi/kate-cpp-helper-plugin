#
# Copyright 2012 by Alex Turbov <i.zaufi@gmail.com>
#
# kate: hl cmake;

cmake_minimum_required(VERSION 2.6)

#---------------------------------------------------------
# Section: Init cmake
#---------------------------------------------------------
project(KateCppHelperPlugin C CXX)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 8)
set(VERSION_PATCH 6)
set(VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

#---------------------------------------------------------
# Section: Include aux cmake functions
#---------------------------------------------------------
include(GuessBuildType)
# TODO Invalid kate highlighting!
if(
    (DEFINED BUILD_TESTING AND BUILD_TESTING) OR
    (NOT DEFINED BUILD_TESTING AND
        (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "DebugFull")
      )
  )
    include(CTest)
endif()

#---------------------------------------------------------
# Section: Find used stuff
#---------------------------------------------------------

# Find KDE
find_package(KDE4 REQUIRED)
include(KDE4Defaults)

find_package(LibClang 3.0 REQUIRED)

# find boost
# NOTE I see no way to make unit tests optional for release mode
# BUILD_TESTING will always turned ON... so caller have to disable
# it explicitly if required.
if (BUILD_TESTING)
    set(NEEDED_BOOST_COMPONENTS unit_test_framework)
else()
    message(STATUS "Unit tests disabled")
endif()
find_package(Boost 1.42 REQUIRED COMPONENTS ${NEEDED_BOOST_COMPONENTS})
if (NOT Boost_UNIT_TEST_FRAMEWORK_LIBRARY)
    message(STATUS "Unit tests disabled")
    set(BUILD_TESTING OFF)
else()
    include(AddBoostTests)
endif()

#---------------------------------------------------------
# Section: Configure building process
#---------------------------------------------------------
# NOTE Order is important!
include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR})
include_directories(${KDE4_INCLUDES} ${QT_INCLUDES} ${Boost_INCLUDE_DIRS})

# Set generic compiler flags: turn ON C++11 (C++03 is suxx) and exceptions
# set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")
add_definitions(-std=c++11 ${KDE4_ENABLE_EXCEPTIONS})

# Tell to boost::result_of to use decltype to decay return type of callable.
# NOTE This would enable to use C++11 labmda expressions w/ boost::rage adaptors
#add_definitions(-DBOOST_RESULT_OF_USE_DECLTYPE)
# Add Qt/KDE defines
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
# Add LLVM definitions
add_definitions(${LLVM_CXXFLAGS})
# Use Kate plugins debug area as default
# add_definitions(-DDEBUG_AREA=13040)

#---------------------------------------------------------
# Section: Define some helpful targets (using included)
#          functions.
# ALERT It should be at the bottom!
#       To make sure all vars r defined!
#---------------------------------------------------------

#include(DefineDoxyDocsTargetIfPossible)
set(NAMING_STYLE Camel)
include(DefineSkeletonGenerationTargetsIfPossible)
include(UpdatePCHFile)
update_pch_header(
    PCH_FILE ${CMAKE_BINARY_DIR}/most_included_files.h
  )

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
