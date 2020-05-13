set(systemmonitor_SRCS
    systemmonitor.cpp
)

add_library(plasma_applet_systemmonitor MODULE ${systemmonitor_SRCS})

kcoreaddons_desktop_to_json(plasma_applet_systemmonitor package/metadata.desktop)

target_link_libraries(plasma_applet_systemmonitor
                      Qt5::Gui
                      Qt5::Qml
                      Qt5::Quick
                      Qt5::DBus
                      KF5::Plasma
                      KF5::I18n
                      KF5::ConfigCore
                      KF5::ConfigGui
                      KF5::Declarative
                      KSysGuard::SysGuard
                      KSysGuard::Sensors
                      KSysGuard::SensorFaces
                     )


install(TARGETS plasma_applet_systemmonitor DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/applets)

plasma_install_package(package org.kde.plasma.systemmonitor)
