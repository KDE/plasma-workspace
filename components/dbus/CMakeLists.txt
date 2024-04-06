# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

ecm_add_qml_module(dbusplugin URI org.kde.plasma.workspace.dbus VERSION 1.0 GENERATE_PLUGIN_SOURCE SHARED)
target_sources(dbusplugin
    PRIVATE
        dbusservicewatcher.cpp
)
target_link_libraries(dbusplugin
    PRIVATE
        Qt::DBus
        Qt::Qml
)
ecm_finalize_qml_module(dbusplugin)

if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
