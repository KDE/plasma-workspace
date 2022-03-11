add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.notifications\")

set(notificationapplet_SRCS
    notificationapplet.cpp
    draghelper.cpp
    fileinfo.cpp
    filemenu.cpp
    globalshortcuts.cpp
    jobaggregator.cpp
    texteditclickhandler.cpp
    thumbnailer.cpp
)

kcoreaddons_add_plugin(plasma_applet_notifications SOURCES ${notificationapplet_SRCS} INSTALL_NAMESPACE "plasma/applets")

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
                      PW::LibNotificationManager
                      )

ecm_qt_declare_logging_category(plasma_applet_notifications
    HEADER notifications_debug.h
    IDENTIFIER PLASMA_APPLET_NOTIFICATIONS_DEBUG
    CATEGORY_NAME org.kde.plasma.notifications)

plasma_install_package(package org.kde.plasma.notifications)
