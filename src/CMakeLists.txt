# kate: hl cmake;

configure_file(config.h.in config.h)
configure_file(katecpphelperplugin.desktop.in katecpphelperplugin.desktop)

add_definitions(-DQT_USE_QSTRINGBUILDER)

set(KATE_CPP_HELPER_PLUGIN_SOURCES
    utils.cpp
    sanitize_snippet.cpp
    plugin_configuration.cpp
    document_info.cpp
    translation_unit.cpp
    clang_utils.cpp
    clang_code_completion_item.cpp
    clang_code_completion_model.cpp
    include_helper_completion_model.cpp
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
  )

kde4_add_plugin(katecpphelperplugin ${KATE_CPP_HELPER_PLUGIN_SOURCES})

target_link_libraries(katecpphelperplugin
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KFILE_LIBS}
    ${KDE4_KTEXTEDITOR_LIBS}
    kateinterfaces
    ${CLANG_C_LIBRARIES}
  )

#
# Generate predefined #include sets for Qt4 and KDE 4.x
#
macro(append_if_set  _var  _list)
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

install(TARGETS katecpphelperplugin DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/katecpphelperplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(
    FILES ui.rc ${CMAKE_CURRENT_BINARY_DIR}/data/qt4.incset
    DESTINATION ${DATA_INSTALL_DIR}/kate/plugins/katecpphelperplugin
  )

if (BUILD_TESTING)
    add_subdirectory(test)
endif()
