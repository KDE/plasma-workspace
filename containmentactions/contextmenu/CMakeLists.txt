project(plasma-containmentactions-contextmenu)

include_directories(${plasma-workspace_SOURCE_DIR}/libkworkspace)

set(contextmenu_SRCS
    menu.cpp
)

set(krunner_xml ${plasma-workspace_SOURCE_DIR}/krunner/dbus/org.kde.krunner.App.xml)
qt5_add_dbus_interface(contextmenu_SRCS ${krunner_xml} krunner_interface)

set(screensaver_xml ${ksmserver_SOURCE_DIR}/screenlocker/dbus/org.freedesktop.ScreenSaver.xml)
qt5_add_dbus_interface(contextmenu_SRCS ${screensaver_xml} screensaver_interface)


add_library(plasma_containmentactions_contextmenu MODULE ${contextmenu_SRCS})

kservice_desktop_to_json(plasma_containmentactions_contextmenu plasma-containmentactions-contextmenu.desktop)

target_link_libraries(plasma_containmentactions_contextmenu
                      Qt5::DBus
                      KF5::I18n
                      KF5::Plasma
                      KF5::XmlGui
                      KF5::KIOCore
                      PW::KWorkspace)

install(TARGETS plasma_containmentactions_contextmenu DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-containmentactions-contextmenu.desktop DESTINATION ${SERVICES_INSTALL_DIR})
