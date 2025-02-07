# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

ecm_add_qml_module(dbusplugin URI org.kde.plasma.workspace.dbus VERSION 1.0 CLASS_NAME DBusPlugin NO_GENERATE_PLUGIN_SOURCE)
target_sources(dbusplugin
    PRIVATE
        dbusplugin.cpp
        dbusconnection.cpp dbusconnection.h
        dbussignalwatcher.cpp dbussignalwatcher.h
        dbusdecoder.cpp dbusdecoder.h
        dbusencoder.cpp dbusencoder.h
        dbuserror.cpp dbuserror.h
        dbusmessage.cpp dbusmessage.h
        dbuspendingreply.cpp dbuspendingreply.h
        dbusproperties.cpp dbusproperties.h
        dbusservicewatcher.cpp dbusservicewatcher.h
        dbustype.h
)
ecm_qt_declare_logging_category(dbusplugin
    HEADER dbusplugin_debug.h
    IDENTIFIER DBUSPLUGIN_DEBUG
    CATEGORY_NAME org.kde.plasma.workspace.dbus
    DESCRIPTION "dbusplugin"
    EXPORT PLASMAWORKSPACE
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
