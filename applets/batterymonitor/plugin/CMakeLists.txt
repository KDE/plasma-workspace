# SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

ecm_add_qml_module(batterycontrolplugin URI org.kde.plasma.private.battery GENERATE_PLUGIN_SOURCE)

target_sources(batterycontrolplugin PRIVATE
        powerprofilescontrol.cpp
        batterycontrol.cpp
        powermanagmentcontrol.cpp
        applicationdata_p.cpp
        batteriesnamesmonitor_p.cpp
        inhibitmonitor_p.cpp
)

target_link_libraries(batterycontrolplugin PRIVATE
        Qt::Core
        Qt::DBus
        Qt::Qml
        Qt::Gui
        KF6::Service
        KF6::Solid
        KF6::I18n
)

ecm_qt_declare_logging_category(batterycontrolplugin
        HEADER "batterymonitor_debug.h"
        IDENTIFIER "APPLETS::BATTERYMONITOR"
        CATEGORY_NAME org.kde.applets.batterymonitor
        DEFAULT_SEVERITY Warning
        DESCRIPTION "Battery Monitor applet" EXPORT "APPLETS::BATTERYMONITOR"
)

ecm_finalize_qml_module(batterycontrolplugin)
