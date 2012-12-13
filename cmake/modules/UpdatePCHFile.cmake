# Copyright (c) 2012 by Alex Turbov
#
# Function to collect most used imported (i.e. not from this project) headers
# and generate a file w/ #include directives for all of them
#
# parameter:
#  PCH_FILE        -- name of target PCH header file to produce
#  EXCLUDE_DIRS    -- list of directories to exclude from scan
#  EXCLUDE_HEADERS -- list of header files to exclude from count
#
function(update_pch_header)
  set(oneValueArgs PCH_FILE)
  set(multiValueArgs EXCLUDE_DIRS EXCLUDE_HEADERS)
  cmake_parse_arguments(update_pch_header "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (update_pch_header_EXCLUDE_DIRS)
    foreach(_dir ${update_pch_header_EXCLUDE_DIRS})
      set(update_pch_header_EXCLUDE_DIRS_REGEX "${update_pch_header_EXCLUDE_DIRS_REGEX}${_pipe}${_dir}")
      set(_pipe "|")
    endforeach()
    unset(_pipe)
    set(
        update_pch_header_FILTER_DIRS_COMMAND
        "COMMAND egrep -v \"(${update_pch_header_EXCLUDE_DIRS_REGEX})/\""
      )
  endif()

  if (update_pch_header_EXCLUDE_HEADERS)
    foreach(_dir ${update_pch_header_EXCLUDE_HEADERS})
      set(update_pch_header_EXCLUDE_HEADERS_REGEX "${update_pch_header_EXCLUDE_HEADERS_REGEX}${_pipe}${_dir}")
      set(_pipe "|")
    endforeach()
    unset(_pipe)
    set(
        update_pch_header_FILTER_FILES_COMMAND
        "COMMAND egrep -v \"(${update_pch_header_EXCLUDE_HEADERS_REGEX})\""
      )
  endif()

  # Render a script to produce a header file w/ most used external headers
  configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/modules/PreparePCHHeader.cmake.in
        ${CMAKE_BINARY_DIR}/cmake/modules/PreparePCHHeader.cmake
        @ONLY
      )

  add_custom_target(
      update-pch-header
      COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/cmake/modules/PreparePCHHeader.cmake
      MAIN_DEPENDENCY cmake/modules/PreparePCHHeader.cmake
      COMMENT "Updating ${update_pch_header_PCH_FILE}"
    )

endfunction()

# kate: hl cmake;
