add_definitions(-UQT_NO_STL)

set(UNIT_TESTS unit_tests)
set(UNIT_TESTS_SOURCES
    sanitize_snippet_tester.cpp
    utils_tester.cpp
    header_files_cache_tester.cpp
    translation_unit_tester.cpp
    ${CMAKE_SOURCE_DIR}/src/sanitize_snippet.cpp
    ${CMAKE_SOURCE_DIR}/src/utils.cpp
    ${CMAKE_SOURCE_DIR}/src/translation_unit.cpp
    ${CLANG_HELPERS_SOURCES}
  )

# Unit tests executable
add_boost_tests(
    TARGET ${UNIT_TESTS}
    SOURCES ${UNIT_TESTS_SOURCES}
  )

target_link_libraries(
    ${UNIT_TESTS}
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${KDE4_KTEXTEDITOR_LIBS}
    ${LIBCLANG_LIBRARY}
  )

# Sample indexer to play with
add_executable(
    indexing_test
    indexing_test.cpp
    ${CLANG_HELPERS_SOURCES}
  )

target_link_libraries(
    indexing_test
    ${KDE4_KDECORE_LIBRARY}
    ${LIBCLANG_LIBRARY}
  )

# Make a symbolic link to data directory
# (convinient for testing purposes)
if(NOT EXISTS ${CMAKE_BINARY_DIR}/src/test/data)
    file(RELATIVE_PATH _dst_data_link "${CMAKE_BINARY_DIR}/src/test" "${CMAKE_SOURCE_DIR}/src/test/data" )
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink "${_dst_data_link}" data
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/test"
      )
endif()

