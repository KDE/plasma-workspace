add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.private.systemtray\")

plasma_install_package(package org.kde.plasma.private.systemtray)

set(systemtray_SRCS
    systemtray.cpp
)

ecm_qt_declare_logging_category(systemtray_SRCS HEADER debug.h
                                            IDENTIFIER SYSTEM_TRAY
                                            CATEGORY_NAME kde.systemtray
                                            DEFAULT_SEVERITY Info)

add_library(org.kde.plasma.private.systemtray MODULE ${systemtray_SRCS})

kcoreaddons_desktop_to_json(org.kde.plasma.private.systemtray package/metadata.desktop)

target_link_libraries(org.kde.plasma.private.systemtray
                      Qt5::Gui
                      Qt5::Quick
                      KF5::Plasma
                      Qt5::DBus
                      KF5::IconThemes
                      KF5::XmlGui
                      KF5::I18n)

install(TARGETS org.kde.plasma.private.systemtray DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/applets)

add_subdirectory(container)
