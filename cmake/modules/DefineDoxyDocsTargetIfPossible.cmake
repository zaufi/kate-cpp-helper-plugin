# - Find `doxygen` (and `mscgen`), render a `Doxyfile` and define
#   a target 'doxygen' to build a project documentation.
#
# To configure desired doxygen options one may set them before
# include this module. Every doxygen variable (found in ordinal Doxyfile)
# must be prefixed w/ `DOXYGEN_` to be defined form CMake script.
#
# This module also define two targets: `doxygen' and `show-api-documentation'.
# Latter uses `xdg-open` to start a default browser and open generated `index.html`.
#
# Tools like `dot` and `mscgen` also will be detected automatically and
# corresponding options in `Doxyfile` will be defined.
#
# NOTE Some options have defaults (reasonable for average project) and some of them,
# appendable -- i.e. being defined in `CMakeLists.txt` default value will be appended
# to the end anyway. Here is a list of "appendable" options:
#  - DOXYGEN_EXCLUDE_PATTERNS
#

#=============================================================================
# Copyright 2011-2014 by Alex Turbov <i.zaufi@gmail.com>
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

option(NO_DOXY_DOCS "Do not install Doxygen'ed documentation")

# check if doxygen is even installed
find_package(Doxygen)

if(DOXYGEN_FOUND STREQUAL "NO")
    message(WARNING "Doxygen not found. Please get a copy http://www.doxygen.org to produce HTML documentation")
else()
    # Try to find `mscgen` as well
    find_program(
        DOXYGEN_MSCGEN_EXECUTABLE
        NAMES mscgen
        DOC "Message Sequence Chart renderer (http://www.mcternan.me.uk/mscgen/)"
      )
    if(DOXYGEN_MSCGEN_EXECUTABLE)
        get_filename_component(DOXYGEN_MSCGEN_PATH "${MSCGEN_EXECUTABLE}" PATH CACHE)
    endif()

    # set some variables before generate a config file
    set(DOXYGEN_HAVE_DOT ${DOXYGEN_DOT_FOUND})
    set(DOXYGEN_STRIP_FROM_PATH "${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR}")

    # override some defaults, but allow to redefine from CMakeLists.txt
    if(NOT DEFINED DOXYGEN_PROJECT_NAME)
        set(DOXYGEN_PROJECT_NAME ${PROJECT_NAME})
    endif()
    if(NOT DEFINED DOXYGEN_PROJECT_NUMBER)
        set(DOXYGEN_PROJECT_NUMBER ${PROJECT_VERSION})
    endif()
    if(NOT DEFINED DOXYGEN_PROJECT_BRIEF)
        set(DOXYGEN_PROJECT_BRIEF "\"${PROJECT_BRIEF}\"")
    endif()
    if(NOT DEFINED DOXYGEN_RECURSIVE)
        set(DOXYGEN_RECURSIVE YES)
    endif()
    if(NOT DEFINED DOXYGEN_INPUT)
        set(DOXYGEN_INPUT "${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR}")
    endif()
    if(NOT DEFINED DOXYGEN_OUTPUT_DIRECTORY)
        set(DOXYGEN_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/doc")
    endif()
    if(NOT DEFINED DOXYGEN_DOT_IMAGE_FORMAT)
        set(DOXYGEN_DOT_IMAGE_FORMAT svg)
    endif()
    if(NOT DEFINED DOXYGEN_INTERACTIVE_SVG)
        set(DOXYGEN_INTERACTIVE_SVG YES)
    endif()
    if(NOT DEFINED DOXYGEN_DOT_TRANSPARENT)
        set(DOXYGEN_DOT_TRANSPARENT YES)
    endif()
    if(NOT DEFINED DOXYGEN_BUILTIN_STL_SUPPORT)
        set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
    endif()
    if(NOT DEFINED DOXYGEN_FULL_PATH_NAMES)
        set(DOXYGEN_FULL_PATH_NAMES YES)
    endif()
    if(NOT DEFINED DOXYGEN_FULL_PATH_NAMES)
        set(DOXYGEN_FULL_PATH_NAMES YES)
    endif()
    if(NOT DEFINED DOXYGEN_SOURCE_BROWSER)
        set(DOXYGEN_SOURCE_BROWSER YES)
    endif()
    if(NOT DEFINED DOXYGEN_STRIP_CODE_COMMENTS)
        set(DOXYGEN_STRIP_CODE_COMMENTS NO)
    endif()
    if(NOT DEFINED DOXYGEN_GENERATE_LATEX)
        set(DOXYGEN_GENERATE_LATEX NO)
    endif()
    if(NOT DEFINED DOXYGEN_DOT_CLEANUP)
        set(DOXYGEN_DOT_CLEANUP NO)
    endif()
    if(NOT DEFINED DOXYGEN_EXTRACT_ALL)
        set(DOXYGEN_EXTRACT_ALL YES)
    endif()
    if(NOT DEFINED DOXYGEN_EXTRACT_PRIVATE)
        set(DOXYGEN_EXTRACT_PRIVATE YES)
    endif()
    if(NOT DEFINED DOXYGEN_EXTRACT_STATIC)
        set(DOXYGEN_EXTRACT_STATIC YES)
    endif()
    if(NOT DEFINED DOXYGEN_EXTRACT_LOCAL_METHODS)
        set(DOXYGEN_EXTRACT_LOCAL_METHODS YES)
    endif()
    if(NOT DEFINED DOXYGEN_INTERNAL_DOCS)
        set(DOXYGEN_INTERNAL_DOCS YES)
    endif()
    if(NOT DEFINED DOXYGEN_SORT_BRIEF_DOCS)
        set(DOXYGEN_SORT_BRIEF_DOCS YES)
    endif()
    if(NOT DEFINED DOXYGEN_SORT_MEMBERS_CTORS_1ST)
        set(DOXYGEN_SORT_MEMBERS_CTORS_1ST YES)
    endif()
    if(NOT DEFINED DOXYGEN_SORT_GROUP_NAMES)
        set(DOXYGEN_SORT_GROUP_NAMES YES)
    endif()
    if(NOT DEFINED DOXYGEN_SORT_BY_SCOPE_NAME)
        set(DOXYGEN_SORT_BY_SCOPE_NAME YES)
    endif()

    # Handle appendable options
    set(
        DOXYGEN_EXCLUDE_PATTERNS
        "${DOXYGEN_EXCLUDE_PATTERNS} */.git/* */.svn/* */.hg/* *_tester.cc */CMakeFiles/* */cmake/* */_CPack_Packages/*"
      )

    # get other defaults from generated file
    include(${CMAKE_CURRENT_LIST_DIR}/DoxygenDefaults.cmake)
    # prepare doxygen configuration file
    configure_file(${CMAKE_CURRENT_LIST_DIR}/Doxyfile.in ${CMAKE_BINARY_DIR}/Doxyfile)

    # add doxygen as target
    add_custom_target(
        doxygen
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
        DEPENDS ${CMAKE_BINARY_DIR}/Doxyfile
        COMMENT "Generate API documentation"
      )

    # cleanup $build/docs on "make clean"
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${DOXYGEN_OUTPUT_DIRECTORY})

    find_program(
        XDG_OPEN_EXECUTABLE
        NAMES xdg-open
        DOC "opens a file or URL in the user's preferred application"
      )
    if(XDG_OPEN_EXECUTABLE)
        message(STATUS "Enable 'show-api-documentation' target via ${XDG_OPEN_EXECUTABLE}")
        add_custom_target(
            show-api-documentation
            COMMAND ${XDG_OPEN_EXECUTABLE} ${DOXYGEN_OUTPUT_DIRECTORY}/html/index.html
            DEPENDS ${CMAKE_BINARY_DIR}/Doxyfile
            COMMENT "Open API documentation"
          )
        add_dependencies(show-api-documentation doxygen)
    endif()

    if(NOT NO_DOXY_DOCS OR NOT NO_DOXY_DOCS STREQUAL "ON")
        # make sure documentation will be produced before (possible) install
        configure_file(
            ${CMAKE_CURRENT_LIST_DIR}/DoxygenInstall.cmake.in
            ${CMAKE_BINARY_DIR}/DoxygenInstall.cmake
            @ONLY
          )
        install(SCRIPT ${CMAKE_BINARY_DIR}/DoxygenInstall.cmake)
    else()
        message(STATUS "Doxygened documentation will not be installed!")
    endif()
endif()

# X-Chewy-RepoBase: https://raw.githubusercontent.com/mutanabbi/chewy-cmake-rep/master/
# X-Chewy-Path: DefineDoxyDocsTargetIfPossible.cmake
# X-Chewy-Version: 2.8
# X-Chewy-Description: Define `make doxygen` target to build API documentation using `doxygen`
# X-Chewy-AddonFile: Doxyfile.in
# X-Chewy-AddonFile: DoxygenInstall.cmake.in
# X-Chewy-AddonFile: DoxygenDefaults.cmake
