add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.notifications\")

set(notificationapplet_SRCS
    notificationapplet.cpp
    fileinfo.cpp
    filemenu.cpp
    globalshortcuts.cpp
    texteditclickhandler.cpp
    thumbnailer.cpp
)

kcoreaddons_add_plugin(plasma_applet_notifications SOURCES ${notificationapplet_SRCS} INSTALL_NAMESPACE "plasma/applets")
kcoreaddons_desktop_to_json(plasma_applet_notifications package/metadata.desktop)

target_link_libraries(plasma_applet_notifications
                      Qt::Gui
                      Qt::Quick # for QQmlParserStatus
                      KF5::ConfigWidgets # for KStandardAction
                      KF5::I18n
                      KF5::Plasma
                      KF5::PlasmaQuick
                      KF5::GlobalAccel
                      KF5::KIOGui
                      KF5::KIOWidgets # for PreviewJob
                      KF5::Notifications # for KNotificationJobUiDelegate
                      )

plasma_install_package(package org.kde.plasma.notifications)
