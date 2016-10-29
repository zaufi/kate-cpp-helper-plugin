# Copyright 2011-2015 by Alex Turbov <i.zaufi@gmail.com>
#
# Command to add a target to make a specified package.
#
# Example:
#
#   add_package(
#       NAME libzencxx
#       SUMMARY "C++11 reusable code collection"
#       DESCRIPTION "Header only libraries"
#       VERSION 0.1-0ubuntu1
#       DEPENDS boost
#       SET_DEFAULT_CONFIG_CPACK
#     )
#
# TODO Add `cpack' programs detection
#
# TODO Add help about generic usage and about set_common_package_options() particularly
#
# TODO One more way to deploy: `dput` to upload to launchpad's PPA
#

#=============================================================================
# Copyright 2011-2015 by Alex Turbov <i.zaufi@gmail.com>
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


include(CMakeParseArguments)

set(_ADD_PACKAGE_MODULE_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(set_common_package_options)
    set(
        one_value_args
            ALL_PACKAGES_TARGET_NAME
            DISTRIB_CODENAME
            HOMEPAGE
            LICENSE_FILE
            PACKAGE_INSTALL_PREFIX
            PROJECT_VERSION
            README_FILE
            SIGN_BY
            SIGN_WITH
            VENDOR_CONTACT
            VENDOR_NAME
      )
    set(multi_value_args)
    cmake_parse_arguments(
        set_common_package_options
        "${options}"
        "${one_value_args}"
        "${multi_value_args}"
        ${ARGN}
      )

    # Check mandatory parameters
    # 0) project version
    if(NOT set_common_package_options_PROJECT_VERSION)
        message(FATAL_ERROR "Project version is not given")
    endif()
    # 1) vendor name
    if(NOT set_common_package_options_VENDOR_NAME)
        message(FATAL_ERROR "Vendor name is not given")
    endif()
    # 2) vendor contact
    if(NOT set_common_package_options_VENDOR_CONTACT)
        message(FATAL_ERROR "Vendor contact is not given")
    endif()
    # Other options are optional...
    if(NOT set_common_package_options_ALL_PACKAGES_TARGET_NAME)
        set(set_common_package_options_ALL_PACKAGES_TARGET_NAME "native-packages")
    endif()
    if(NOT set_common_package_options_README_FILE)
        if(EXISTS "${CMAKE_SOURCE_DIR}/README")
            set(set_common_package_options_README_FILE "${CMAKE_SOURCE_DIR}/README")
        elseif(EXISTS "${CMAKE_SOURCE_DIR}/README.md")
            set(set_common_package_options_README_FILE "${CMAKE_SOURCE_DIR}/README.md")
        else()                                              # TODO Check other files?
            set(set_common_package_options_README_FILE "")
        endif()
    endif()
    if(NOT set_common_package_options_LICENSE_FILE)
        if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
            set(set_common_package_options_LICENSE_FILE "${CMAKE_SOURCE_DIR}/LICENSE")
        else()
            set(set_common_package_options_LICENSE_FILE "")
        endif()
    endif()
    if(NOT set_common_package_options_PACKAGE_INSTALL_PREFIX)
        # Reasonable default for all packages in any distro
        set(set_common_package_options_PACKAGE_INSTALL_PREFIX "/usr")
    endif()
    # Check if we can produce signed packages
    if(set_common_package_options_SIGN_WITH AND set_common_package_options_SIGN_BY)
        message(
            STATUS
            "Packages will be signed by ${set_common_package_options_SIGN_BY} [${set_common_package_options_SIGN_WITH}]"
          )
        # Try to find utilities required to sign packages
        foreach(_gen ${CPACK_GENERATOR})
            if(_gen STREQUAL "DEB")
                # Try to use `dpkg-sign` for Debian
                find_program(DPKG_SIGN_EXECUTABLE dpkg-sig)
                if(DPKG_SIGN_EXECUTABLE)
                    set(DPKG_SIGN_EXECUTABLE "${DPKG_SIGN_EXECUTABLE}" PARENT_SCOPE)
                else()
                    message(STATUS "WARNING: `dpkg-sig' executable not found. Packages will not be signed!")
                endif()
            elseif(_gen STREQUAL "RPM")
                # Try to use `rpm` for CentOS/RHEL
                find_program(RPM_SIGN_EXECUTABLE rpmsign)
                if(RPM_SIGN_EXECUTABLE)
                    set(RPM_SIGN_EXECUTABLE "${RPM_SIGN_EXECUTABLE}" PARENT_SCOPE)
                else()
                    message(STATUS "WARNING: `rpmsign' executable not found. Packages will not be signed!")
                endif()
            endif()
        endforeach()
    elseif(NOT (set_common_package_options_SIGN_WITH AND set_common_package_options_SIGN_BY))
        message(STATUS "NOTE Packages will not be signed")
    else()
        message(FATAL_ERROR "Both SIGN_BY and SIGN_WITH options must be provided or none of them")
    endif()

    # Generate a common package options file 2b included from
    # all particilar package control files generated by add_package()
    configure_file(
        ${_ADD_PACKAGE_MODULE_BASE_DIR}/CPackCommonPackageOptions.cmake.in
        ${CMAKE_BINARY_DIR}/CPackCommonPackageOptions.cmake
      )
endfunction()

function(add_package)
    set(options SET_DEFAULT_CONFIG_CPACK)
    set(
        one_value_args
            ARCHITECTURE
            CHANGELOG_FILE
            DESCRIPTION
            NAME
            POST_INSTALL_SCRIPT_FILE
            POST_UNINSTALL_SCRIPT_FILE
            PRE_INSTALL_SCRIPT_FILE
            PRE_UNINSTALL_SCRIPT_FILE
            SECTION
            SUMMARY
            VERSION
      )
    set(
        multi_value_args
            BREAKS
            CONFLICTS
            CONTROL_FILES
            DEPENDS
            ENHANCES
            EXCLUDE_FROM_AUTO_FILELIST
            OBSOLETES
            PRE_BUILD
            PRE_DEPENDS
            PRIORITY
            PROVIDES
            RECOMMENDS
            REPLACES
            REQUIRES
            SUGGESTS
            USER_FILELIST
      )
    cmake_parse_arguments(add_package "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    # Make sure set_common_package_options() was called before
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/CPackCommonPackageOptions.cmake)
        message(FATAL_ERROR "set_common_package_options() must be called before any of add_package()")
    endif()
    # (Re)use generated file to get some common vars into the current scope
    include(${CMAKE_BINARY_DIR}/CPackCommonPackageOptions.cmake)

    # Check mandatory parameters
    # 0) package name
    if(NOT add_package_NAME)
        message(FATAL_ERROR "Package name is not given")
    else()
        # Generate component name to be used by install() calls in CMake's files
        string(TOUPPER "${add_package_NAME}" pkg_name)
        string(REPLACE "-" "_" pkg_name "${pkg_name}")
        set(${pkg_name}_PACKAGE "${add_package_NAME}" PARENT_SCOPE)
    endif()
    # 1) package description
    if(NOT add_package_SUMMARY OR NOT add_package_DESCRIPTION)
        message(FATAL_ERROR "Package description and/or summary is not provided")
    endif()

    # Check optional parameters
    if (add_package_SET_DEFAULT_CONFIG_CPACK)
        set(config_file CPackConfig.cmake)
    else()
        set(config_file CPack-${add_package_NAME}.cmake)
    endif()
    # package version
    if(NOT add_package_VERSION)
        message(FATAL_ERROR "Package version is not provided")
    endif()
    # various dependency lists
    # NOTE Form a comma separated list of dependencies from a cmake's list
    # as required by Debian/RPM control/spec file
    if(add_package_BREAKS)
        string(REPLACE ";" ", " add_package_BREAKS "${add_package_BREAKS}")
    endif()
    if(add_package_CONFLICTS)
        string(REPLACE ";" ", " add_package_CONFLICTS "${add_package_CONFLICTS}")
    endif()
    if(add_package_DEPENDS)
        string(REPLACE ";" ", " add_package_DEPENDS "${add_package_DEPENDS}")
    endif()
    if(add_package_ENHANCES)
        string(REPLACE ";" ", " add_package_ENHANCES "${add_package_ENHANCES}")
    endif()
    if(add_package_OBSOLETES)
        string(REPLACE ";" ", " add_package_OBSOLETES "${add_package_OBSOLETES}")
    endif()
    if(add_package_PRE_DEPENDS)
        string(REPLACE ";" ", " add_package_PRE_DEPENDS "${add_package_PRE_DEPENDS}")
    endif()
    if(add_package_PROVIDES)
        string(REPLACE ";" ", " add_package_PROVIDES "${add_package_PROVIDES}")
    endif()
    if(add_package_RECOMMENDS)
        string(REPLACE ";" ", " add_package_RECOMMENDS "${add_package_RECOMMENDS}")
    endif()
    if(add_package_REPLACES)
        string(REPLACE ";" ", " add_package_REPLACES "${add_package_REPLACES}")
    endif()
    if(add_package_REQUIRES)
        string(REPLACE ";" ", " add_package_REQUIRES "${add_package_REQUIRES}")
    endif()
    if(add_package_SUGGESTS)
        string(REPLACE ";" ", " add_package_SUGGESTS "${add_package_SUGGESTS}")
    endif()

    # Generate a package specific cpack's config file to be used
    # at custom execution command...
    configure_file(
        ${_ADD_PACKAGE_MODULE_BASE_DIR}/CPackPackageConfig.cmake.in
        ${CMAKE_BINARY_DIR}/${config_file}
      )

    # Add a commulative target for all packages if not defined yet
    if(NOT TARGET ${CPACK_ALL_PACKAGES_TARGET_NAME})
        add_custom_target(${CPACK_ALL_PACKAGES_TARGET_NAME})
    endif()

    # Iterate over `CPACK_GENERATOR`s and add particular package target
    foreach(_gen ${CPACK_GENERATOR})
        string(TOLOWER "${_gen}" _lgen)
        if(CPACK_PACKAGE_SIGN_KEY_ID AND ${_gen} MATCHES "DEB" AND DPKG_SIGN_EXECUTABLE)
            set(_make_pkg_target_name "signed-${add_package_NAME}-${_lgen}-package")
            add_custom_target(
                ${_make_pkg_target_name}
                COMMAND ${CMAKE_CPACK_COMMAND} -G ${_gen} --config ${config_file}
                COMMAND ${DPKG_SIGN_EXECUTABLE} -s "${CPACK_PACKAGE_SIGNER}" -k "${CPACK_PACKAGE_SIGN_KEY_ID}" ${add_package_FILE_NAME}.deb
                DEPENDS
                    ${add_package_PRE_BUILD}
                    ${CMAKE_BINARY_DIR}/CPackCommonPackageOptions.cmake
                    ${CMAKE_BINARY_DIR}/${config_file}
                COMMENT "Making signed package ${add_package_NAME}"
              )
            add_dependencies(${CPACK_ALL_PACKAGES_TARGET_NAME} ${_make_pkg_target_name})
        elseif(CPACK_PACKAGE_SIGN_KEY_ID AND ${_gen} MATCHES "RPM" AND RPM_SIGN_EXECUTABLE)
            set(_make_pkg_target_name "signed-${add_package_NAME}-${_lgen}-package")
            add_custom_target(
                ${_make_pkg_target_name}
                COMMAND ${CMAKE_CPACK_COMMAND} -G ${_gen} --config ${config_file}
                COMMAND ${RPM_SIGN_EXECUTABLE} --addsign --key-id="${CPACK_PACKAGE_SIGNER}" ${add_package_FILE_NAME}.rpm
                DEPENDS
                    ${add_package_PRE_BUILD}
                    ${CMAKE_BINARY_DIR}/CPackCommonPackageOptions.cmake
                    ${CMAKE_BINARY_DIR}/${config_file}
                COMMENT "Making signed package ${add_package_NAME}"
              )
            add_dependencies(${CPACK_ALL_PACKAGES_TARGET_NAME} ${_make_pkg_target_name})
        else()
            set(_make_pkg_target_name "${add_package_NAME}-${_lgen}-package")
            add_custom_target(
                ${_make_pkg_target_name}
                COMMAND ${CMAKE_CPACK_COMMAND} -G ${_gen} --config ${config_file}
                DEPENDS
                    ${add_package_PRE_BUILD}
                    ${CMAKE_BINARY_DIR}/CPackCommonPackageOptions.cmake
                    ${CMAKE_BINARY_DIR}/${config_file}
                COMMENT "Making package ${add_package_NAME}"
              )
            add_dependencies(${CPACK_ALL_PACKAGES_TARGET_NAME} ${_make_pkg_target_name})
        endif()
    endforeach()
endfunction()

# X-Chewy-RepoBase: https://raw.githubusercontent.com/mutanabbi/chewy-cmake-rep/master/
# X-Chewy-Path: AddPackage.cmake
# X-Chewy-Version: 4.6
# X-Chewy-Description: Add a target to make a .deb/.rpm package
# X-Chewy-AddonFile: CPackCommonPackageOptions.cmake.in
# X-Chewy-AddonFile: CPackPackageConfig.cmake.in
