# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

ecm_add_qml_module(dbusplugin URI org.kde.plasma.workspace.dbus VERSION 1.0 CLASS_NAME DBusPlugin NO_GENERATE_PLUGIN_SOURCE)
target_sources(dbusplugin
    PRIVATE
        dbusplugin.cpp
        dbusconnection.cpp
        dbusdecoder.cpp
        dbusencoder.cpp
        dbuserror.cpp
        dbusmessage.cpp
        dbuspendingreply.cpp
        dbusservicewatcher.cpp
        dbustype.h
)
target_link_libraries(dbusplugin
    PRIVATE
        Qt::DBus
        Qt::Qml
        PkgConfig::DBus
)
ecm_finalize_qml_module(dbusplugin)

if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
