kcoreaddons_add_plugin(org.kde.plasma.systemmonitor SOURCES systemmonitor.cpp systemmonitor.h INSTALL_NAMESPACE "plasma/applets")

target_link_libraries(org.kde.plasma.systemmonitor
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
