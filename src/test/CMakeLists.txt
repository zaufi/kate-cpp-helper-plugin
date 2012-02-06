set(UNIT_TESTS unit_tests)
set(UNIT_TESTS_SOURCES
    utils_tester.cpp
    ${CMAKE_SOURCE_DIR}/src/utils.cpp
  )

# Unit tests executable
add_executable(${UNIT_TESTS} ${UNIT_TESTS_SOURCES})
target_link_libraries(${UNIT_TESTS}
    ${KDE4_KTEXTEDITOR_LIBS}
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
  )
add_boost_tests(${UNIT_TESTS} "--catch_system_errors=no" ${UNIT_TESTS_SOURCES})
