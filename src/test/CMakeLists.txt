add_definitions(-UQT_NO_STL)

#
# Unit tests executable
#
add_boost_tests(
    TARGET unit_tests
    MODULE BasicTests
    DYN_LINK
    SOURCES
        sanitize_snippet_tester.cpp
        utils_tester.cpp
        header_files_cache_tester.cpp
        translation_unit_tester.cpp
        database_manager_tester.cpp
        serialize_tester.cpp
        index_utils_tester.cpp
        unsaved_files_list_tester.cpp
  )

target_link_libraries(
    unit_tests
    # NOTE Due a single pass GNU linker `sharedcode4testsmoc` target (library)
    # must be specified twice: before and after `sharedcode4tests`
    sharedcode4tests
    sharedcode4testsmoc
    sharedcode4tests
    sharedcode4testsmoc
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SERIALIZATION_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${KDE4_KTEXTEDITOR_LIBS}
    ${KDE4_KFILE_LIBS}
    ${LIBCLANG_LIBRARY}
    ${XAPIAN_LIBRARIES}
  )

#
# QTest based indexer unit-tests
#
qt4_wrap_cpp(INDEXER_TESTER_HEADERS_MOC indexer_tester.h)
set(
    INDEXER_UNIT_TEST_SOURCES
    indexer_tester.cpp
    ${INDEXER_TESTER_HEADERS_MOC}
  )

add_executable(
    indexer_unit_tests
    ${INDEXER_UNIT_TEST_SOURCES}
  )

target_link_libraries(
    indexer_unit_tests
    sharedcode4tests
    sharedcode4testsmoc
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SERIALIZATION_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${KDE4_KDEUI_LIBRARY}
    ${LIBCLANG_LIBRARY}
    ${QT_QTTEST_LIBRARY_RELEASE}
    ${XAPIAN_LIBRARIES}
  )

#
# Sample indexer to play w/ it
#
add_executable(
    indexing_test
    indexing_test.cpp
  )

target_link_libraries(
    indexing_test
    sharedcode4tests
    ${KDE4_KDECORE_LIBRARY}
    ${LIBCLANG_LIBRARY}
  )

configure_file(data/test_manifest.in data/fake.db/manifest)
