add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.notifications\")

set(notificationapplet_SRCS
    notificationapplet.cpp
    notificationwindow.cpp
    draghelper.cpp
    fileinfo.cpp
    filemenu.cpp
    globalshortcuts.cpp
    jobaggregator.cpp
    thumbnailer.cpp
    notificationapplet.h
    notificationwindow.h
    draghelper.h
    fileinfo.h
    filemenu.h
    globalshortcuts.h
    jobaggregator.h
    thumbnailer.h
)

kcoreaddons_add_plugin(org.kde.plasma.notifications SOURCES ${notificationapplet_SRCS} INSTALL_NAMESPACE "plasma/applets")


if (Qt6_VERSION VERSION_GREATER_EQUAL "6.8.0")
    set(private_code_option  "PRIVATE_CODE")
endif()
qt_generate_wayland_protocol_client_sources(org.kde.plasma.notifications
    FILES
        "${PLASMA_WAYLAND_PROTOCOLS_DIR}/plasma-shell.xml"
        "${Wayland_DATADIR}/wayland.xml"
    ${private_code_option}
)

target_link_libraries(org.kde.plasma.notifications
                      Qt::Gui
                      Qt::Quick # for QQmlParserStatus
                      KF6::ConfigWidgets # for KStandardAction
                      KF6::I18n
                      Plasma::Plasma
                      Plasma::PlasmaQuick
                      KF6::CoreAddons
                      KF6::GlobalAccel
                      KF6::KIOGui
                      KF6::KIOWidgets # for PreviewJob
                      KF6::Notifications # for KNotificationJobUiDelegate
                      PW::LibNotificationManager
                      Wayland::Client
                      )

ecm_qt_declare_logging_category(org.kde.plasma.notifications
    HEADER notifications_debug.h
    IDENTIFIER PLASMA_APPLET_NOTIFICATIONS_DEBUG
    CATEGORY_NAME org.kde.plasma.notifications
    DESCRIPTION "plasma applet notification"
    EXPORT PLASMAWORKSPACE
)

plasma_install_package(package org.kde.plasma.notifications)
