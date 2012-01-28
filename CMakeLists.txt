project(ktexteditor_includehelper)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include_directories(${KDE4_INCLUDES} ${QT_INCLUDES})
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

set(ktexteditor_includehelper_SRCS includehelperplugin.cpp)

kde4_add_plugin(ktexteditor_includehelper ${ktexteditor_includehelper_SRCS})

target_link_libraries(ktexteditor_includehelper ${KDE4_KDEUI_LIBS} ${KDE4_KTEXTEDITOR_LIBS})

install(TARGETS ktexteditor_includehelper DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES includehelperui.rc DESTINATION ${DATA_INSTALL_DIR}/ktexteditor_includehelper)

install(FILES ktexteditor_includehelper.desktop DESTINATION ${SERVICES_INSTALL_DIR})
 
