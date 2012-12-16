# kate: hl cmake;

configure_file(config.h.in config.h)
configure_file(katecpphelperplugin.desktop.in katecpphelperplugin.desktop)

add_definitions(-DQT_USE_QSTRINGBUILDER)

set(KATE_CPP_HELPER_PLUGIN_SOURCES
    utils.cpp
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
  )

kde4_add_plugin(katecpphelperplugin ${KATE_CPP_HELPER_PLUGIN_SOURCES})

target_link_libraries(katecpphelperplugin
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KFILE_LIBS}
    ${KDE4_KTEXTEDITOR_LIBS}
    kateinterfaces
    ${CLANG_C_LIBRARIES}
  )

install(TARGETS katecpphelperplugin DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES ui.rc DESTINATION ${DATA_INSTALL_DIR}/katecpphelperplugin)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/katecpphelperplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR})

if (BUILD_TESTING)
    add_subdirectory(test)
endif()
