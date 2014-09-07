#
# Copyright 2012-2014 by Alex Turbov <i.zaufi@gmail.com>
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
set(VERSION_PATCH 2)
if(BUILD_NUMBER)
    set(VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${BUILD_NUMBER})
else()
    set(VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
endif()
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

#---------------------------------------------------------
# Section: Include aux cmake functions
#---------------------------------------------------------
include(GuessBuildType)

# TODO Invalid kate highlighting!
if(
    (DEFINED BUILD_TESTING AND BUILD_TESTING) OR
    (NOT DEFINED BUILD_TESTING AND
        (CMAKE_BUILD_TYPE MATCHES "Debug" AND IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.git")
      )
  )
    include(CTest)
    include(AddCompileTests)
    set(_REAL_BUILD_TESTING ON)
else()
    set(_REAL_BUILD_TESTING OFF)
endif()

#---------------------------------------------------------
# Section: Find used stuff
#---------------------------------------------------------

# Find KDE
find_package(KDE4 REQUIRED)
include(KDE4Defaults)
# ATTENTION BUILD_TESTING turned unconditionally ON by KDE4Defaults,
# so return to the previous state after this include()
set(BUILD_TESTING ${_REAL_BUILD_TESTING})

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

# Provide some options to be set by user
set(WITH_SANITIZER "" CACHE STRING "Instrument code with sanitizer(s) (comma separated list)")
string(REPLACE "," ";" WITH_SANITIZER "${WITH_SANITIZER}")

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
if(UNIX AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Allow GNU extensions
    add_definitions(-D_GNU_SOURCE)

    # BOOST_PP (boost <= 1.55.0) do not recognize clang as a compiler w/ variadic macros support,
    # so lets force it!
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_definitions(-DBOOST_PP_VARIADICS=1)
    endif()

    # Include declaration of use_compiler_option()
    include(UseCompilerOption)
    # Set some compiler options
    use_compiler_option(-pipe OUTPUT COMPILER_HAS_PIPE_OPTION)
    use_compiler_option(-Wall OUTPUT COMPILER_HAS_WALL_OPTION)
    use_compiler_option(-Wextra OUTPUT COMPILER_HAS_WEXTRA_OPTION)
    # TODO I don't know a minimum required version...
    # Please provide some feedback, if lesser works for you.
    use_compiler_option(-std=c++1y LANGUAGE CXX OUTPUT CXX_COMPILER_HAS_CPP1Y_OPTION)
    if(NOT CXX_COMPILER_HAS_CPP1Y_OPTION)
        message(FATAL_ERROR "C++14 compatible compiler required to build this project (gcc >= 4.9 or clang >= 3.5)")
    endif()

    # Do not link w/ libraries which isn't provide undefined symbols.
    # (they are specified as dependencies for other targets from this
    # project, but listed as unused by `ldd -u binary`)
    use_compiler_option("-Wl,--as-needed" OUTPUT LINKER_HAS_AS_NEEDED)
    use_compiler_option("-Wl,--no-undefined" OUTPUT LINKER_HAS_NO_UNDEFINED)

    # Setup compiler options depending on build type
    message(STATUS "Setting options for ${CMAKE_BUILD_TYPE} build type")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "DebugFull")
        # Show even more warnings in debug mode
        use_compiler_option(-Wextra OUTPUT COMPILER_HAS_WALL_OPTION)
        use_compiler_option(-ggdb3 OUTPUT COMPILER_HAS_GGDB3_OPTION)
        if(NOT COMPILER_HAS_GGDB3_OPTION)
            use_compiler_option(-g3 OUTPUT COMPILER_HAS_G3_OPTION)
        endif()
    else()
        # More linker optimizations in release mode
        use_compiler_option("-Wl,-O1" OUTPUT LINKER_HAS_O1)
        use_compiler_option("-Wl,--sort-common" OUTPUT LINKER_HAS_SORT_COMMON)
        # TODO Provide more compiler options (like -march or -ftree-vectorize)
        # TODO Use LTO
    endif()
endif()

# ... and allow to use exceptions w/ Qt code
add_definitions(${KDE4_ENABLE_EXCEPTIONS})

# Tell to boost::result_of to use decltype to decay return type of callable.
# NOTE This would enable to use C++11 labmda expressions w/ boost::rage adaptors
add_definitions(-DBOOST_RESULT_OF_USE_DECLTYPE)
# Don't want to use any deprecated API
add_definitions(-DBOOST_SYSTEM_NO_DEPRECATED)
add_definitions(-DBOOST_FILESYSTEM_NO_DEPRECATED)
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
# Section: Top level installs
#---------------------------------------------------------
install(
    FILES README.md LICENSE Changes.md
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/katecpphelperplugin
    COMPONENT ${KATE_CPP_HELPER_PLUGIN_PACKAGE}
  )
install(
    FILES contrib/sample.sanitizer-rules
    DESTINATION ${DATA_INSTALL_DIR}/kate/plugins/katecpphelperplugin
    COMPONENT ${KATE_CPP_HELPER_PLUGIN_PACKAGE}
  )

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
    "build"
    "\\\\.travis\\\\.yml"
  )

# Include CPack support
include(CPack)
if(DISTRIB_PKG_FMT STREQUAL "DEB")
    set(CPACK_GENERATOR ${DISTRIB_PKG_FMT})
    set(REPREPRO_CUSTOM_OPTIONS "--ask-passphrase")
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
    set(
        _DEB_REPOSITORY
        "$ENV{HOME}/ubuntu.repo"
        CACHE FILEPATH "Path to repository to deploy .deb packages (using `reprepro`)"
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
        ALLOW_DEPLOY_PACKAGES
        DEPLOY_TO ${_DEB_REPOSITORY}
      )

    # Obtain distribution codename according LSB
    include(GetDistribInfo)

    # Specifying precise versions of some packages which are distinct
    # in different distributions.
    if(DISTRIB_CODENAME STREQUAL "precise")
        set(BOOST_DEV_VERSION_PKG_DEP "1.48")
    elseif(DISTRIB_CODENAME STREQUAL "quantal")
        set(BOOST_DEV_VERSION_PKG_DEP "1.50")
    elseif(DISTRIB_CODENAME STREQUAL "raring")
        set(BOOST_DEV_VERSION_PKG_DEP "1.53")
    elseif(DISTRIB_CODENAME STREQUAL "saucy")
        set(BOOST_DEV_VERSION_PKG_DEP "1.54")
    else()
        message(STATUS "WARNING: Target distribution codename is unknown!")
    endif()
    set(BOOST_VERSION_PKG_DEP "${BOOST_DEV_VERSION_PKG_DEP}.0")

    add_package(
        NAME kate-cpp-helper-plugin
        SUMMARY "Kate C++ Helper Plugin"
        DESCRIPTION "Plugin for KDE Advanced Text Editor"
        HOMEPAGE "http://zaufi.github.io/kate-cpp-helper-plugin.html"
        VERSION "${VERSION_STRING}-0ubuntu1"
        SECTION "editors"
        DEPENDS
            "libboost-filesystem${BOOST_VERSION_PKG_DEP}"
            "libboost-system${BOOST_VERSION_PKG_DEP}"
            "libboost-serialization${BOOST_VERSION_PKG_DEP}"
            "libclang1-3.3"
            "libxapian22"
            "kate"
        REPLACES "kate-cpp-helper-plugin${CPACK_BUILD_FLAVOUR} (<= ${VERSION_STRING})"
     )
endif()

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

# Setup doxygen generator
set(DOXYGEN_PROJECT_BRIEF "\"Kate C++ Helper Plugin\"")
set(DOXYGEN_PROJECT_NUMBER ${VERSION_STRING})
set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
set(DOXYGEN_FULL_PATH_NAMES YES)
set(DOXYGEN_FULL_PATH_NAMES YES)
set(DOXYGEN_SOURCE_BROWSER YES)
set(DOXYGEN_STRIP_CODE_COMMENTS NO)
set(DOXYGEN_GENERATE_LATEX NO)
set(DOXYGEN_DOT_CLEANUP NO)
set(DOXYGEN_EXTRACT_ALL YES)
set(DOXYGEN_EXTRACT_PRIVATE YES)
set(DOXYGEN_EXTRACT_STATIC YES)
set(DOXYGEN_EXTRACT_LOCAL_METHODS YES)
set(DOXYGEN_INTERNAL_DOCS YES)
set(DOXYGEN_SORT_BRIEF_DOCS YES)
set(DOXYGEN_SORT_MEMBERS_CTORS_1ST YES)
set(DOXYGEN_SORT_GROUP_NAMES YES)
set(DOXYGEN_SORT_BY_SCOPE_NAME YES)
if(UNIX AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND EXISTS $ENV{HOME}/.cache/doxygen/libstdc++.tag)
    # TODO Download libstdc++ tag file
    set(
        DOXYGEN_TAGFILES
            "$ENV{HOME}/.cache/doxygen/libstdc++.tag=http://gcc.gnu.org/onlinedocs/libstdc++/latest-doxygen/"
        )
endif()
include(GenerateDoxygenDocumentation)
generate_doxygen_documentation(doxygen)

# Declare target to open generated API documentation
include(AddOpenTarget)
add_open_target(
    show-api-dox
    "${CMAKE_CURRENT_BINARY_DIR}/doc/html/index.html"
    DEPENDS doxygen
    )

include(DefineSkeletonGenerationTargetsIfPossible)
define_skeleton_generation_targets(
    PROJECT_LICENSE GPL
    PROJECT_NAMESPACE kate
    PROJECT_OWNER "Alex Turbov"
    PROJECT_YEARS "2011-2014"
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
