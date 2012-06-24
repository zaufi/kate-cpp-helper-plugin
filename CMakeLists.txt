#
# Copyright 2012 by Alex Turbov <i.zaufi@gmail.com>
#
# kate: hl cmake;

#---------------------------------------------------------
# Section: Init cmake
#---------------------------------------------------------
project(KateIncludeHelperPlugin C CXX)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 4)
set(VERSION_PATCH 1)
set(VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

#---------------------------------------------------------
# Section: Include aux cmake functions
#---------------------------------------------------------
include(CTest)
include(AddBoostTests)

#---------------------------------------------------------
# Section: Find used stuff
#---------------------------------------------------------

# Find KDE
find_package(KDE4 REQUIRED)
include(KDE4Defaults)

# find boost
if (BUILD_TESTING)
    find_package(Boost 1.42 REQUIRED COMPONENTS
        unit_test_framework
      )
endif()

#---------------------------------------------------------
# Section: Configure building process
#---------------------------------------------------------
# NOTE Order is important!
include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR})
include_directories(${KDE4_INCLUDES} ${QT_INCLUDES} ${Boost_INCLUDE_DIRS})

# Tell to boost::result_of to use decltype to decay return type of callable.
# NOTE This would enabel to use C++11 labmda expressions w/ boost::rage adaptors
#add_definitions(-DBOOST_RESULT_OF_USE_DECLTYPE)
# Add Qt/KDE defines
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
# Use Kate plugins debug area as default
# add_definitions(-DDEBUG_AREA=13040)

# Set generic compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

if(FORCE_BUG_WORKAROUND)
    add_definitions(-DIHP_FORCE_BUG_WORKAROUND)
endif()

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
