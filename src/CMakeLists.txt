configure_file(config.h.in config.h)
configure_file(katecpphelperplugin.desktop.in katecpphelperplugin.desktop)

#
# Make a library for unit tests
#
set(LIBTEST_SOURCES
    clang/compiler_options.cpp
    clang/location.cpp
    clang/to_string.cpp
    clang/unsaved_files_list.cpp
    index/utils.cpp
    index/database.cpp
    index/combined_index.cpp
    index/indexer.cpp
    indexing_targets_list_model.cpp
    indices_table_model.cpp
    search_results_table_model.cpp
    database_manager.cpp
    sanitize_snippet.cpp
    utils.cpp
    translation_unit.cpp
  )

qt4_wrap_cpp(
    LIBTEST_SOURCES_MOC
    index/indexer.h
    indexing_targets_list_model.h
    indices_table_model.h
    search_results_table_model.h
    database_manager.h
  )

kde4_add_kcfg_files(LIBTEST_SOURCES database_options.kcfgc)

if (BUILD_TESTING)
    add_library(sharedcode4tests STATIC ${LIBTEST_SOURCES})
    add_library(sharedcode4testsmoc STATIC ${LIBTEST_SOURCES_MOC})
endif()


#
# Make a plugin DSO
#
set(KATE_CPP_HELPER_PLUGIN_SOURCES
    ${LIBTEST_SOURCES}
    document_info.cpp
    document_proxy.cpp
    plugin_configuration.cpp
    diagnostic_messages_model.cpp
    clang_code_completion_item.cpp
    clang_code_completion_model.cpp
    include_helper_completion_model.cpp
    choose_from_list_dialog.cpp
    cpp_helper_plugin_config_page.cpp
    cpp_helper_plugin_view.cpp
    cpp_helper_plugin.cpp
  )

kde4_add_ui_files(
    KATE_CPP_HELPER_PLUGIN_SOURCES
    path_config.ui
    clang_settings.ui
    other_settings.ui
    detect_compiler_paths.ui
    session_paths_sets.ui
    plugin_tool_view.ui
    clang_completion_settings.ui
  )

kde4_add_plugin(katecpphelperplugin ${KATE_CPP_HELPER_PLUGIN_SOURCES})

target_link_libraries(katecpphelperplugin
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SERIALIZATION_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${KATE_INTERFACES_LIB}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KFILE_LIBS}
    ${KDE4_KTEXTEDITOR_LIBS}
    ${LIBCLANG_LIBRARY}
    ${XAPIAN_LIBRARIES}
  )

#
# Generate predefined #include sets for Qt4 and KDE 4.x
#
macro(append_if_set _var _list)
    if(${_var})
        if(${_list})
            set(${_list} "${${_list}},${${_var}}")
        else()
            set(${_list} "${${_var}}")
        endif()
    endif()
endmacro()

append_if_set(QT_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QT3SUPPORT_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTASSISTANT_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTASSISTANTCLIENT_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QAXCONTAINER_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QAXSERVER_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTCORE_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTDBUS_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTDESIGNER_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTDESIGNERCOMPONENTS_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTGUI_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTHELP_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTMOTIF_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTMULTIMEDIA_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTNETWORK_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTNSPLUGIN_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTOPENGL_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTSCRIPT_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTSQL_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTSVG_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTTEST_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTWEBKIT_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTXML_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTXMLPATTERNS_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_PHONON_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTSCRIPTTOOLS_INCLUDE_DIR QT4_INCSET_DIRS)
append_if_set(QT_QTDECLARATIVE_INCLUDE_DIR QT4_INCSET_DIRS)

configure_file(data/qt4.incset.in data/qt4.incset)

#
# Install targets
#
install(FILES database_options.kcfg DESTINATION ${KCFG_INSTALL_DIR})

install(
    TARGETS katecpphelperplugin
    DESTINATION ${PLUGIN_INSTALL_DIR}
    COMPONENT ${KATE_CPP_HELPER_PLUGIN_PACKAGE}
  )
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/katecpphelperplugin.desktop
    DESTINATION ${SERVICES_INSTALL_DIR}
    COMPONENT ${KATE_CPP_HELPER_PLUGIN_PACKAGE}
  )
install(
    FILES ui.rc ${CMAKE_CURRENT_BINARY_DIR}/data/qt4.incset
    DESTINATION ${DATA_INSTALL_DIR}/kate/plugins/katecpphelperplugin
    COMPONENT ${KATE_CPP_HELPER_PLUGIN_PACKAGE}
  )

if (BUILD_TESTING)
    add_subdirectory(test)
endif()
