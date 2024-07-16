# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.devicenotifier\")

ecm_add_qml_module(devicenotifierplugin URI org.kde.plasma.private.devicenotifier GENERATE_PLUGIN_SOURCE)

target_sources(devicenotifierplugin
    PRIVATE
        actionscontrol.cpp
        devicecontrol.cpp
        spacemonitor_p.cpp
        devicestatemonitor_p.cpp
        deviceserviceaction.cpp
        predicatesmonitor_p.cpp
        deviceerrormonitor_p.cpp
        actioninterface.cpp
        devicefiltercontrol.cpp
        actions/defaultaction.cpp
        actions/mountandopenaction.cpp
        actions/mountaction.cpp
        actions/unmountaction.cpp
        actions/openwithfilemanageraction.cpp
)

target_link_libraries(devicenotifierplugin
    PRIVATE
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

ecm_qt_declare_logging_category(devicenotifierplugin
        HEADER "devicenotifier_debug.h"
        IDENTIFIER "APPLETS::DEVICENOTIFIER"
        CATEGORY_NAME org.kde.applets.devicenotifier
        DEFAULT_SEVERITY Warning
        DESCRIPTION "Device Notifier applet" EXPORT "APPLETS::DEVICENOTIFIER"
)

ecm_finalize_qml_module(devicenotifierplugin)

install(FILES devicenotifications.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
