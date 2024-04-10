/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: MIT
*/

import QtQuick
import org.kde.plasma.workspace.dbus as DBus

Item {
    width: 100
    height: 100
    property alias caller: caller
    DBus.DBusMethodCall {
        id: caller
        busType: DBus.BusType.Session
        service: "org.freedesktop.Notifications"
        objectPath: "/org/freedesktop/Notifications"
        iface: "org.freedesktop.Notifications"
        method: "Notify"
        inSignature: "(susssasa{sv}i)"
        arguments: [
            "appname",
            0,
            "firefox",
            "title",
            "body",
            ["1", "action 1", "2", "action 2"],
            {"desktop-entry": "systemsettings"},
            -1
        ]
        onSuccess: reply => console.log(reply)
        onError: message => console.error(message)
    }
    TapHandler {
        onTapped: caller.call()
    }
}
