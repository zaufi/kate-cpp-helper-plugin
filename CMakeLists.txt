#
# Copyright 2012-2013 by Alex Turbov <i.zaufi@gmail.com>
#

# TODO I don't know a minimum required version...
# Please provide some feedback, if lesser works for you.
cmake_minimum_required(VERSION 2.8)

#---------------------------------------------------------
# Section: Init cmake
#---------------------------------------------------------
project(KateCppHelperPlugin C CXX)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)
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

# Find kate developer files
# See BUG #10
include(CheckIncludeFileCXX)
set(
    KATE_INTERFACE_FILES
    kate/application.h
    kate/documentmanager.h
    kate/mainwindow.h
    kate/pluginconfigpageinterface.h
    kate/plugin.h
  )
set(CMAKE_REQUIRED_INCLUDES ${KDE4_INCLUDES} ${QT_INCLUDES})
set(_need_to_include_kate_interfaces OFF)
foreach(_kate_iface_file ${KATE_INTERFACE_FILES})
    get_filename_component(_file_name ${_kate_iface_file} NAME_WE)
    string(TOUPPER "HAVE_KATE_${_file_name}_H" _have_var)
    check_include_file_cxx(${_kate_iface_file} ${_have_var})
    if(NOT ${_have_var})
        set(_need_to_include_kate_interfaces ON)
    endif()
endforeach()

find_library(KATE_INTERFACES_LIB NAMES libkateinterfaces.so libkateinterfaces.so.4)

# TODO I don't know a minimum required version...
# Please provide some feedback, if lesser works for you.
find_package(LibClang 3.3 REQUIRED)

# find boost
set(NEEDED_BOOST_COMPONENTS
    filesystem
    serialization
    system
  )
# NOTE I see no way to make unit tests optional for release mode
# BUILD_TESTING will always turned ON... so caller have to disable
# it explicitly if required.
if(BUILD_TESTING)
    set(NEEDED_BOOST_COMPONENTS ${NEEDED_BOOST_COMPONENTS} unit_test_framework)
else()
    message(STATUS "Unit tests disabled")
endif()

# TODO I don't know a minimum required version...
# Please provide some feedback, if lesser works for you.
# ATTENTION 1.49 should be quite enough
find_package(Boost 1.49 REQUIRED COMPONENTS ${NEEDED_BOOST_COMPONENTS})

if(NOT Boost_UNIT_TEST_FRAMEWORK_LIBRARY)
    message(STATUS "Unit tests disabled")
    set(BUILD_TESTING OFF)
else()
    include(AddBoostTests)
endif()

# TODO I don't know a minimum required version...
# Please provide some feedback, if lesser works for you.
find_package(Xapian 1.2.12 REQUIRED)
message(STATUS "Found Xapian library: ${XAPIAN_LIBRARIES}")

#---------------------------------------------------------
# Section: Configure building process
#---------------------------------------------------------
# NOTE Order is important!
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_BINARY_DIR}
    ${KDE4_INCLUDES}
    ${QT_INCLUDES}
    ${Boost_INCLUDE_DIRS}
    ${LLVM_INCLUDEDIR}
    ${XAPIAN_INCLUDE_DIR}
  )

if(_need_to_include_kate_interfaces)
    if("${KDE_VERSION}" VERSION_LESS 4.8)
        message(FATAL_ERROR "KDE >=4.8.x required to build this package")
    elseif("${KDE_VERSION}" VERSION_LESS 4.10)
        set(_use_kate_iface_ver "${KDE_VERSION_MAJOR}.${KDE_VERSION_MINOR}")
    else()
        set(_use_kate_iface_ver "latest")
    endif()
    include_directories(
        ${PROJECT_SOURCE_DIR}/third_party/kde${KDE_VERSION_MAJOR}-kate-interfaces/${_use_kate_iface_ver}
      )
    if(_use_kate_iface_ver STREQUAL "latest")
        set(_use_kate_iface_ver ">=4.10")
    else()
        set(_use_kate_iface_ver "=${_use_kate_iface_ver}")
    endif()
    message(STATUS "Use bundled kate interface files for KDE/kate ${_use_kate_iface_ver}")
endif()

# Set generic compiler flags: turn ON C++11 (C++03 is suxx)...
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # TODO I don't know a minimum required version...
    # Please provide some feedback, if lesser works for you.
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.8.0")
        message(FATAL_ERROR "GNU gcc >=4.8.0 is required to build this package")
    else()
        set(CXX11_FLAG "-std=c++11")
    endif()
endif()
# ... and allow to use exceptions w/ Qt code
add_definitions(${CXX11_FLAG} ${KDE4_ENABLE_EXCEPTIONS})

# Tell to boost::result_of to use decltype to decay return type of callable.
# NOTE This would enable to use C++11 labmda expressions w/ boost::rage adaptors
add_definitions(-DBOOST_RESULT_OF_USE_DECLTYPE)
# Add Qt/KDE defines
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
# Use expression templates to build QStrings
add_definitions(-DQT_USE_QSTRINGBUILDER)
# NOTE Disable trivial Qt keywords due conflicts w/ some Xapian headers
add_definitions(-DQT_NO_KEYWORDS)
# Add LLVM definitions
add_definitions(${LLVM_CXXFLAGS})
# Use Kate plugins debug area as default
# TODO Register private debug area?
add_definitions(-DDEBUG_AREA=13040)

# Keep RPATH to found libraries (particularly LLVM's) in installed .so file
# BUG https://github.com/zaufi/kate-cpp-helper-plugin/issues/6
set(CMAKE_INSTALL_RPATH "${LLVM_LIBDIR}")

# Render configuration file (used by unit tests)
configure_file(config.h.in config.h @ONLY)

#---------------------------------------------------------
# Section: Configure package options
#---------------------------------------------------------
set(CPACK_SOURCE_GENERATOR TBZ2)
set(
    CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${VERSION_STRING}"
    CACHE INTERNAL "tarball basename"
  )
# The following components are regex's to match anywhere (unless anchored)
# in absolute path + filename to find files or directories to be excluded
# from source tarball.
set(CPACK_SOURCE_IGNORE_FILES
    "\\\\.gitignore$"
    "\\\\.kate-swp$"
    "\\\\.tar\\\\.gz$"
    "^.*/\\\\.git.*$"
    "\\\\.kcrash\\\\.txt$"
    "^~.*$"
  )

# Include CPack support
include(CPack)
set(CPACK_GENERATOR DEB)
include(AddPackage)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "DebugFull")
    set(CPACK_BUILD_FLAVOUR "-dbg")
else()
    set(CPACK_BUILD_FLAVOUR "")
endif()

set(
    _SIGN_KEY_ID
    "37925FE2"
    CACHE STRING "GPG key ID to be used to sign result packages"
  )
set(
    _SIGN_USER_NAME
    "builder"
    CACHE STRING "User name to be used to sign packages (builder)"
  )

set_common_package_options(
    PROJECT_VERSION "${VERSION_STRING}"
    VENDOR_NAME "Alex Turbov"
    VENDOR_CONTACT "Alex Turbov <i.zaufi@gmail.com>"
    BUILD_FLAVOUR "${CPACK_BUILD_FLAVOUR}"
    PACKAGE_INSTALL_PREFIX "/usr"
    README_FILE ${PROJECT_SOURCE_DIR}/README.md
    LICENSE_FILE ${PROJECT_SOURCE_DIR}/LICENSE
    SIGN_BY ${_SIGN_USER_NAME}
    SIGN_WITH ${_SIGN_KEY_ID}
  )

add_package(
    NAME kate-cpp-helper-plugin
    SUMMARY "Kate C++ Helper Plugin"
    DESCRIPTION "Plugin for KDE Advanced Text Editor"
    HOMEPAGE "http://zaufi.github.io/kate-cpp-helper-plugin.html"
    VERSION "${VERSION_STRING}-0ubuntu1"
    SECTION "editors"
    DEPENDS
        "libstdc++6"
        "libclang1 >= 3.3"
        "libxapian22"
        "kate"
    REPLACES "kate-cpp-helper-plugin${CPACK_BUILD_FLAVOUR} (<= ${VERSION_STRING})"
  )

#---------------------------------------------------------
# Section: Dive into subdirs after main configuration
#---------------------------------------------------------
add_subdirectory(src)

#---------------------------------------------------------
# Section: Define some helpful targets (using included)
#          functions.
# ALERT It should be at the bottom!
#       To make sure all vars r defined!
#---------------------------------------------------------

# Setup doxygen
set(DOXYGEN_PROJECT_BRIEF "\"Kate C++ Helper Plugin\"")
set(DOXYGEN_TAGFILES "~/.cache/doxygen/libstdc++.tag=http://gcc.gnu.org/onlinedocs/libstdc++/latest-doxygen/")
set(DOXYGEN_PROJECT_NUMBER ${VERSION_STRING})
include(DefineDoxyDocsTargetIfPossible)

include(DefineSkeletonGenerationTargetsIfPossible)
define_skeleton_generation_targets(
    PROJECT_LICENSE GPL
    PROJECT_NAMESPACE kate
    PROJECT_OWNER "Alex Turbov"
    PROJECT_YEARS "2011-2013"
    HEADER_EXT "h"
    IMPL_EXT "cpp"
    ENABLE_TESTS
    USE_CAMEL_STYLE
    USE_PRAGMA_ONCE
  )

include(UsePCHFile)
use_pch_file(
    PCH_FILE ${CMAKE_BINARY_DIR}/most_included_files.h
  )

#---------------------------------------------------------
# Section: Top level installs
#---------------------------------------------------------
install(
    FILES README.md LICENSE Changes.md
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/katecpphelperplugin
    COMPONENT ${KATE_CPP_HELPER_PLUGIN_PACKAGE}
  )
