add_definitions(-UQT_NO_STL)
set(UNIT_TESTS unit_tests)
set(UNIT_TESTS_SOURCES
    sanitize_snippet_tester.cpp
    ${CMAKE_SOURCE_DIR}/src/sanitize_snippet.cpp
    utils_tester.cpp
    ${CMAKE_SOURCE_DIR}/src/utils.cpp
    header_files_cache_tester.cpp
  )

# Unit tests executable
add_executable(
    ${UNIT_TESTS}
    ${UNIT_TESTS_SOURCES}
  )
target_link_libraries(
    ${UNIT_TESTS}
    ${KDE4_KTEXTEDITOR_LIBS}
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
  )
add_boost_tests(
    TARGET ${UNIT_TESTS}
    SOURCES ${UNIT_TESTS_SOURCES}
  )
