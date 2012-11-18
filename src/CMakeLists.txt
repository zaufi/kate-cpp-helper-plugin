configure_file(config.h.in config.h)
configure_file(kateincludehelperplugin.desktop.in kateincludehelperplugin.desktop)

set(KATE_INCLUDE_HELPER_PLUGIN_SOURCES
    utils.cpp
    document_info.cpp
    clang_code_completion_model.cpp
    include_helper_plugin_config_page.cpp
    include_helper_plugin_completion_model.cpp
    include_helper_plugin_view.cpp
    include_helper_plugin.cpp
  )

kde4_add_ui_files(KATE_INCLUDE_HELPER_PLUGIN_SOURCES path_config.ui clang_settings.ui other_settings.ui)

kde4_add_plugin(kateincludehelperplugin ${KATE_INCLUDE_HELPER_PLUGIN_SOURCES})

target_link_libraries(kateincludehelperplugin
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KFILE_LIBS}
    ${KDE4_KTEXTEDITOR_LIBS}
    kateinterfaces
    ${CLANG_C_LIBS} ${LLVM_LDFLAGS}
  )

install(TARGETS kateincludehelperplugin DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES ui.rc DESTINATION ${DATA_INSTALL_DIR}/kateincludehelperplugin)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kateincludehelperplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR})

if (BUILD_TESTING)
    add_subdirectory(test)
endif()
