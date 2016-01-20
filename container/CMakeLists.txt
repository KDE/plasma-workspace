
plasma_install_package(package org.kde.plasma.systraycontainer)

set(systemtraycontainer_SRCS
    systemtraycontainer.cpp
)

add_library(plasma_applet_systemtraycontainer MODULE ${systemtraycontainer_SRCS})

kcoreaddons_desktop_to_json(plasma_applet_systemtraycontainer package/metadata.desktop)

target_link_libraries(plasma_applet_systemtraycontainer
                      Qt5::Gui
                      Qt5::Quick
                      KF5::Plasma
                      KF5::XmlGui
                      KF5::I18n)


install(TARGETS plasma_applet_systemtraycontainer DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/applets)


