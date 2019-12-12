add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.notifications\")

set(notificationapplet_SRCS
    notificationapplet.cpp
    filemenu.cpp
    globalshortcuts.cpp
    thumbnailer.cpp
)

add_library(plasma_applet_notifications MODULE ${notificationapplet_SRCS})

kcoreaddons_desktop_to_json(plasma_applet_notifications package/metadata.desktop)

target_link_libraries(plasma_applet_notifications
                      Qt5::Gui
                      Qt5::Quick # for QQmlParserStatus
                      KF5::I18n
                      KF5::Plasma
                      KF5::PlasmaQuick
                      KF5::GlobalAccel
                      KF5::KIOWidgets # for PreviewJob
                      )

install(TARGETS plasma_applet_notifications DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/applets)

plasma_install_package(package org.kde.plasma.notifications)
