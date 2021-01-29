add_definitions(-DTRANSLATION_DOMAIN=\"plasma_containmentactions_contextmenu\")

include_directories(${plasma-workspace_SOURCE_DIR}/libkworkspace)

set(contextmenu_SRCS
    menu.cpp
)

set(krunner_xml ${plasma-workspace_SOURCE_DIR}/krunner/dbus/org.kde.krunner.App.xml)
qt5_add_dbus_interface(contextmenu_SRCS ${krunner_xml} krunner_interface)

qt5_add_dbus_interface(contextmenu_SRCS ${SCREENSAVER_DBUS_INTERFACE} screensaver_interface)


add_library(plasma_containmentactions_contextmenu MODULE ${contextmenu_SRCS})

kcoreaddons_desktop_to_json(plasma_containmentactions_contextmenu plasma-containmentactions-contextmenu.desktop)

target_link_libraries(plasma_containmentactions_contextmenu
                      Qt::DBus
                      KF5::I18n
                      KF5::GlobalAccel
                      KF5::Plasma
                      KF5::XmlGui
                      KF5::KIOCore
                      PW::KWorkspace)

install(TARGETS plasma_containmentactions_contextmenu DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/containmentactions)
