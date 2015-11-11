add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.symplesystray\")

plasma_install_package(package org.kde.plasma.simplesystray)

set(systemtray_SRCS
    systemtray.cpp
)

add_library(plasma_applet_systemtray MODULE ${systemtray_SRCS})

kcoreaddons_desktop_to_json(plasma_applet_systemtray package/metadata.desktop)

target_link_libraries(plasma_applet_systemtray
                      Qt5::Gui
                      KF5::Plasma
                      KF5::I18n)


install(TARGETS plasma_applet_systemtray DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/applets)

