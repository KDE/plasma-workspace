# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.devicenotifier\")

plasma_add_applet(org.kde.plasma.devicenotifier
        QML_SOURCES
        qml/DeviceItem.qml
        qml/FullRepresentation.qml
        qml/main.qml
        CPP_SOURCES
        devicecontrol.cpp
        storageinfo.cpp
        stateinfo.cpp
        spaceinfo.cpp
        messageinfo.cpp
        actionsinfo.cpp
        spaceupdatemonitor_p.cpp
        deviceserviceaction.cpp
        predicatesmonitor_p.cpp
        actioninterface.cpp
        devicefiltercontrol.cpp
        actions/defaultaction.cpp
        actions/mountandopenaction.cpp
        actions/mountaction.cpp
        actions/unmountaction.cpp
        actions/openwithfilemanageraction.cpp
        actions/checkaction.cpp
        RESOURCES
        main.xml
        GENERATE_APPLET_CLASS
)

target_link_libraries(org.kde.plasma.devicenotifier
    PRIVATE
        Qt::DBus
        Qt::Qml
        Plasma::Plasma
        KF6::Solid
        KF6::I18n
        KF6::CoreAddons
        KF6::Service
        KF6::KIOCore
        KF6::KIOGui # KIO::CommandLauncherJob
        KF6::JobWidgets # KNotificationJobUiDelegate
        KSysGuard::ProcessCore
        KF6::Notifications
)

ecm_qt_declare_logging_category(org.kde.plasma.devicenotifier
        HEADER "devicenotifier_debug.h"
        IDENTIFIER "APPLETS::DEVICENOTIFIER"
        CATEGORY_NAME org.kde.applets.devicenotifier
        DEFAULT_SEVERITY Warning
        DESCRIPTION "Device Notifier applet" EXPORT "APPLETS::DEVICENOTIFIER"
)

install(FILES devicenotifications.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
install(FILES openWithFileManager.desktop DESTINATION ${KDE_INSTALL_DATADIR}/solid/actions)
