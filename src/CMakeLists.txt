set(KATE_INCLUDE_HELPER_PLUGIN_SOURCES
    document_info.cpp
    include_helper_plugin_config_page.cpp
    include_helper_plugin_completion_model.cpp
    include_helper_plugin_view.cpp
    include_helper_plugin.cpp
    utils.cpp
  )

kde4_add_ui_files(KATE_INCLUDE_HELPER_PLUGIN_SOURCES configuration.ui)

kde4_add_plugin(kateincludehelperplugin ${KATE_INCLUDE_HELPER_PLUGIN_SOURCES})

target_link_libraries(kateincludehelperplugin
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KFILE_LIBS}
    ${KDE4_KTEXTEDITOR_LIBS}
    kateinterfaces
  )

install(TARGETS kateincludehelperplugin DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES ui.rc DESTINATION ${DATA_INSTALL_DIR}/kateincludehelperplugin)
install(FILES kateincludehelperplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR})

if (BUILD_TESTING)
    add_subdirectory(test)
endif()
