/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: MIT
*/

import QtQuick
import org.kde.plasma.workspace.dbus as DBus

Item {
    width: 100
    height: 100
    property alias watcher: watcher
    DBus.DBusServiceWatcher {
        id: watcher
        busType: DBus.BusType.Session
        watchedService: "org.kde.KSplash"
    }
}
