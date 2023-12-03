# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.devicenotifier\")

ecm_add_qml_module(devicenotifierplugin URI org.kde.plasma.private.devicenotifier GENERATE_PLUGIN_SOURCE)
target_sources(devicenotifierplugin
    PRIVATE
        ksolidnotify.cpp
)

target_link_libraries(devicenotifierplugin
    PRIVATE
        Qt::Qml
        KF6::Solid
        KF6::I18n
        KSysGuard::ProcessCore
        KF6::Notifications
)

ecm_finalize_qml_module(devicenotifierplugin)

install(FILES devicenotifications.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
