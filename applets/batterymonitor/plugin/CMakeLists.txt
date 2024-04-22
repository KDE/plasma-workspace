# SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

ecm_add_qml_module(batterymonitorplugin URI org.kde.plasma.private.batterymonitor GENERATE_PLUGIN_SOURCE)

target_sources(batterymonitorplugin PRIVATE
        powerprofilescontrol.cpp
        powermanagementcontrol.cpp
        applicationdata_p.cpp
        inhibitmonitor_p.cpp
)

target_link_libraries(batterymonitorplugin PRIVATE
        Qt::Core
        Qt::DBus
        Qt::Qml
        Qt::Gui
        KF6::Service
        KF6::I18n
)

ecm_qt_declare_logging_category(batterymonitorplugin
        HEADER "batterymonitor_debug.h"
        IDENTIFIER "APPLETS::BATTERYMONITOR"
        CATEGORY_NAME org.kde.applets.batterymonitor
        DEFAULT_SEVERITY Warning
        DESCRIPTION "Battery Monitor applet" EXPORT "APPLETS::BATTERYMONITOR"
)

ecm_finalize_qml_module(batterymonitorplugin)
