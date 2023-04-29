add_definitions(-DTRANSLATION_DOMAIN=\"krdb\")

add_library(krdb krdb.cpp)

generate_export_header(krdb)

target_include_directories(krdb PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>" "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>" "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/krdb>")

target_link_libraries(krdb PRIVATE Qt::Widgets Qt::DBus KF6::CoreAddons KF6::DBusAddons KF6::GuiAddons KF6::I18n KF6::WindowSystem KF6::ConfigWidgets X11::X11 Qt::GuiPrivate)

install(TARGETS krdb ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)

install(FILES krdb.h
    ${CMAKE_CURRENT_BINARY_DIR}/krdb_export.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/krdb COMPONENT Devel
)
