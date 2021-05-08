kcoreaddons_add_plugin(plasma_applet_systemmonitor SOURCES systemmonitor.cpp INSTALL_NAMESPACE "plasma/applets")
kcoreaddons_desktop_to_json(plasma_applet_systemmonitor package/metadata.desktop)

target_link_libraries(plasma_applet_systemmonitor
                      Qt::Gui
                      Qt::Qml
                      Qt::Quick
                      Qt::DBus
                      KF5::Plasma
                      KF5::I18n
                      KF5::ConfigCore
                      KF5::ConfigGui
                      KF5::Declarative
                      KF5::KIOGui
                      KF5::Notifications
                      KSysGuard::SysGuard
                      KSysGuard::Sensors
                      KSysGuard::SensorFaces
                     )

plasma_install_package(package org.kde.plasma.systemmonitor)
