# Copyright 2011-2013 by Alex Turbov <i.zaufi@gmail.com>
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

include(CMakeParseArguments)

set(_ADD_PACKAGE_MODULE_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")


function(set_common_package_options)
    set(options ALLOW_DEPLOY_PACKAGES)
    set(
        one_value_args
            ARCHITECTURE
            BUILD_FLAVOUR
            DEPLOY_TO
            LICENSE_FILE
            PACKAGE_INSTALL_PREFIX
            PROJECT_VERSION
            README_FILE
            REPOSITORY_COMPONENT
            DISTRIB_CODENAME
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
    if(NOT set_common_package_options_BUILD_FLAVOUR)
        set(set_common_package_options_BUILD_FLAVOUR "")
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
        find_program(DPKG_SIGN_EXECUTABLE dpkg-sig)
        if(DPKG_SIGN_EXECUTABLE)
            message(STATUS "Will sign packages using key ${set_common_package_options_SIGN_WITH} with signing name '${set_common_package_options_SIGN_BY}'")
            set(CPACK_SIGN_KEY_ID "${set_common_package_options_SIGN_WITH}" PARENT_SCOPE)
            set(DPKG_SIGN_EXECUTABLE "${DPKG_SIGN_EXECUTABLE}" PARENT_SCOPE)
        else()
            message(STATUS "WARNING: `dpkg-sig' executable not found. Packages will not be signed!")
        endif()
    else()
        message(FATAL_ERROR "Both SIGN_BY and SIGN_WITH options must be provided or none of them")
    endif()
    # Could we add deploy targets?
    if(set_common_package_options_ALLOW_DEPLOY_PACKAGES)
        find_program(REPREPRO_EXECUTABLE reprepro)
        if(REPREPRO_EXECUTABLE)
            set(REPREPRO_EXECUTABLE "${REPREPRO_EXECUTABLE}" PARENT_SCOPE)
            # Try to get/guess destination repository
            if(NOT set_common_package_options_DEPLOY_TO)
                set(_repo_hint $ENV{HOME}/repo $ENV{HOME}/public_html/repo)
            else()
                set(_repo_hint ${set_common_package_options_DEPLOY_TO})
            endif()
            find_path(
                _use_repo
                conf/distributions
                PATHS ${_repo_hint}
                NO_DEFAULT_PATH
              )
            if(NOT _use_repo STREQUAL _use_repo-NOTFOUND)
                message(STATUS "Enable deploy packages to the repository: ${_use_repo}")
                set(set_common_package_options_USE_REPO "${_use_repo}")
                if(NOT set_common_package_options_REPO_COMPONENT)
                    set(set_common_package_options_REPO_COMPONENT "main")
                endif()
                # Distribution codename
                if(NOT set_common_package_options_DISTRIB_CODENAME)
                    include(GetDistribInfo)
                    set(set_common_package_options_DISTRIB_CODENAME "${DISTRIB_CODENAME}")
                endif()
            else()
                message(STATUS "WARNING: repository path not found! You'll be unable to deploy generated .deb packages!")
            endif()
        else()
            message(STATUS "WARNING: `reprepro' executable not found. Deploy targets disabled.")
        endif()
    endif()
    # Architecture: optional but must be defined anyway
    if(NOT set_common_package_options_ARCHITECTURE)
        # There is no such thing as i686 architecture on debian, you should use i386 instead
        # $ dpkg --print-architecture
        find_program(DPKG_EXECUTABLE dpkg)
        if(NOT DPKG_EXECUTABLE)
            # TODO Detect an architecture based on `uname` output
            message(STATUS "Can not find `dpkg' in your path, setting to amd64.")
            set(set_common_package_options_ARCHITECTURE amd64)
        endif()
        execute_process(COMMAND "${DPKG_EXECUTABLE}" --print-architecture
            OUTPUT_VARIABLE set_common_package_options_ARCHITECTURE
            OUTPUT_STRIP_TRAILING_WHITESPACE
          )
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
            DESCRIPTION
            HOMEPAGE
            NAME
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
            PRE_BUILD
            PRE_DEPENDS
            PRIORITY
            RECOMMENDS
            REPLACES
            SUGGESTS
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
        set(add_package_VERSION "${CPACK_PROJECT_VERSION}-0ubuntu1")
    endif()
    # various dependency lists
    # NOTE Form a comma separated list of dependencies from a cmake's list
    # as required by Debian control file
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
    if(add_package_PRE_DEPENDS)
        string(REPLACE ";" ", " add_package_PRE_DEPENDS "${add_package_PRE_DEPENDS}")
    endif()
    if(add_package_RECOMMENDS)
        string(REPLACE ";" ", " add_package_RECOMMENDS "${add_package_RECOMMENDS}")
    endif()
    if(add_package_REPLACES)
        string(REPLACE ";" ", " add_package_REPLACES "${add_package_REPLACES}")
    endif()
    if(add_package_SUGGESTS)
        string(REPLACE ";" ", " add_package_SUGGESTS "${add_package_SUGGESTS}")
    endif()

    # Define package filename
    set(
        add_package_FILE_NAME
        "${add_package_NAME}${CPACK_BUILD_FLAVOUR}_${add_package_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}"
      )

    # Generate a package specific cpack's config file to be used
    # at custom execution command...
    configure_file(
        ${_ADD_PACKAGE_MODULE_BASE_DIR}/CPackPackageConfig.cmake.in
        ${CMAKE_BINARY_DIR}/${config_file}
      )
    foreach(_gen ${CPACK_GENERATOR})
        string(TOLOWER "${_gen}" _lgen)
        # ATTENTION Signed targets can be produced only for Debian packages nowadays
        if(CPACK_SIGN_KEY_ID AND ${_gen} MATCHES "DEB")
            set(_make_pkg_target_name "signed-${add_package_NAME}-${_lgen}-package")
            add_custom_target(
                ${_make_pkg_target_name}
                COMMAND cpack -G ${_gen} --config ${config_file}
                COMMAND ${DPKG_SIGN_EXECUTABLE} -s ${CPACK_PACKAGE_SIGNER} -k ${CPACK_SIGN_KEY_ID} ${add_package_FILE_NAME}.deb
                DEPENDS
                    ${add_package_PRE_BUILD}
                    ${CMAKE_BINARY_DIR}/CPackCommonPackageOptions.cmake
                    ${CMAKE_BINARY_DIR}/${config_file}
                COMMENT "Making signed package ${add_package_NAME}"
              )
        else()
            set(_make_pkg_target_name "${add_package_NAME}-${_lgen}-package")
            add_custom_target(
                ${_make_pkg_target_name}
                COMMAND cpack -G ${_gen} --config ${config_file}
                DEPENDS
                    ${add_package_PRE_BUILD}
                    ${CMAKE_BINARY_DIR}/CPackCommonPackageOptions.cmake
                    ${CMAKE_BINARY_DIR}/${config_file}
                COMMENT "Making package ${add_package_NAME}"
              )
        endif()
        if(${_gen} MATCHES "DEB" AND REPREPRO_EXECUTABLE)
            add_custom_target(
                deploy-${add_package_NAME}-${_lgen}-package
                COMMAND ${REPREPRO_EXECUTABLE}
                    -b ${CPACK_DEB_PACKAGES_REPO}
                    -C ${CPACK_DEB_PACKAGES_REPO_COMPONENT}
                    ${REPREPRO_CUSTOM_OPTIONS}
                    includedeb ${CPACK_DISTRIB_CODENAME} ${add_package_FILE_NAME}.deb
                DEPENDS ${_make_pkg_target_name}
                COMMENT "Deploying package ${add_package_NAME} to ${CPACK_DEB_PACKAGES_REPO} [${CPACK_DISTRIB_CODENAME}/${CPACK_DEB_PACKAGES_REPO_COMPONENT}]"
              )
        endif()
    endforeach()
endfunction()

# X-Chewy-RepoBase: https://raw.github.com/mutanabbi/chewy-cmake-rep/master/
# X-Chewy-Path: AddPackage.cmake
# X-Chewy-Version: 3.12
# X-Chewy-Description: Add a target to make a .deb package
# X-Chewy-AddonFile: CPackCommonPackageOptions.cmake.in
# X-Chewy-AddonFile: CPackPackageConfig.cmake.in
