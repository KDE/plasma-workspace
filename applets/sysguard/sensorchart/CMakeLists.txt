set(sensorchart_SRCS
    sensorchart.cpp
)

add_library(plasma_applet_ksysguard_sensorchart MODULE ${sensorchart_SRCS})

kcoreaddons_desktop_to_json(plasma_applet_ksysguard_sensorchart package/metadata.desktop)

target_link_libraries(plasma_applet_ksysguard_sensorchart
                      Qt5::Gui
                      Qt5::Qml
                      Qt5::Quick
                      Qt5::DBus
                      KF5::Plasma
                      KF5::Package
                      KF5::I18n
                      KF5::ConfigCore
                      KF5::ConfigGui
                      KF5::Declarative
                      KF5::SysGuard
                      KF5::Sensors
                      KF5::NewStuff
                     )


install(TARGETS plasma_applet_ksysguard_sensorchart DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/applets)

plasma_install_package(package org.kde.ksysguard.sensorchart)

install(FILES sensorchart-faces.knsrc sensorchart-presets.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})

add_subdirectory(packagestructure)
add_subdirectory(faces)
