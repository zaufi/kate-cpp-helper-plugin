#
# Copyright 2010 Entell Solutions
#

#---------------------------------------------------------
# Section: Init cmake
#---------------------------------------------------------
project(kate_include_helper_plugin C CXX)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 4)
set(VERSION_PATCH 0)
set(VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

#---------------------------------------------------------
# Section: Include aux cmake functions
#---------------------------------------------------------

#---------------------------------------------------------
# Section: Find used stuff
#---------------------------------------------------------

# Find KDE
find_package(KDE4 REQUIRED)
include(KDE4Defaults)

# find boost
find_package(Boost 1.46.1 REQUIRED COMPONENTS
    # Lets keep libs list sorted... :)
    filesystem
  )

#---------------------------------------------------------
# Section: Configure building process
#---------------------------------------------------------
# NOTE Order is important!
include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR})
include_directories(${KDE4_INCLUDES} ${QT_INCLUDES} ${Boost_INCLUDE_DIRS})

# Tell to boost::result_of to use decltype to decay return type of callable.
# NOTE This would enabel to use C++11 labmda expressions w/ boost::rage adaptors
add_definitions(-DBOOST_RESULT_OF_USE_DECLTYPE)
# Add Qt/KDE defines
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
# Use Kate plugins debug area as default
# add_definitions(-DDEBUG_AREA=13040)

# Do not link w/ libraries which isn't provide undefined symbols.
# (they are specified as dependencies for other targets from this
# project, but listed as unused by `ldd -u binary`)
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed ${CMAKE_EXE_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--as-needed ${CMAKE_MODULE_LINKER_FLAGS}")
# Set generic compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -std=c++0x ${KDE4_ENABLE_EXCEPTIONS}")

# Setup compiler options depending on build type
message(STATUS "Setting options for ${CMAKE_BUILD_TYPE} build type")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  # Show all warnings in debug mode
  set(CMAKE_CXX_FLAGS_DEBUG "-ggdb3 -Wall")
  # Allow additional security measures
  add_definitions(-D_FORTIFY_SOURCE=2)
else()
  # More linker optimizations in release mode
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE
      "-Wl,-O1 -Wl,--sort-common ${CMAKE_EXE_LINKER_FLAGS_RELEASE}"
    )
  set(CMAKE_MODULE_LINKER_FLAGS_RELEASE
      "-Wl,-O1 -Wl,--sort-common ${CMAKE_MODULE_LINKER_FLAGS_RELEASE}"
    )
endif()

#---------------------------------------------------------
# Section: Define some helpful targets (using included)
#          functions.
# ALERT It should be at the bottom!
#       To make sure all vars r defined!
#---------------------------------------------------------

#include(DefineDoxyDocsTargetIfPossible)
include(DefineSkeletonGenerationTargetsIfPossible)

#---------------------------------------------------------
# Section: Dive into subdirs after main configuration
#---------------------------------------------------------
add_subdirectory(src)
