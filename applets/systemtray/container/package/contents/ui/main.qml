/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.1
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.plasmoid 2.0

//SystemTray is a Containment. To have it presented as a widget in Plasma we need thin wrapping applet
Item {
    id: root

    Layout.minimumWidth: internalSystray ? internalSystray.Layout.minimumWidth : 0
    Layout.minimumHeight: internalSystray ? internalSystray.Layout.minimumHeight : 0
    Layout.preferredWidth: Layout.minimumWidth
    Layout.preferredHeight: Layout.minimumHeight

    Plasmoid.preferredRepresentation: Plasmoid.fullRepresentation
    Plasmoid.status: internalSystray ? internalSystray.status : PlasmaCore.Types.UnknownStatus

    //synchronize state between SystemTray and wrapping Applet
    Plasmoid.onExpandedChanged: {
        if (internalSystray) {
            internalSystray.expanded = Plasmoid.expanded
        }
    }
    Connections {
        target: internalSystray
        function onExpandedChanged() {
            Plasmoid.expanded = internalSystray.expanded
        }
    }

    property Item internalSystray

    Component.onCompleted: {
        root.internalSystray = Plasmoid.nativeInterface.internalSystray;

        if (root.internalSystray == null) {
            return;
        }
        root.internalSystray.parent = root;
        root.internalSystray.anchors.fill = root;
    }

    Connections {
        target: Plasmoid.nativeInterface
        function onInternalSystrayChanged() {
            root.internalSystray = Plasmoid.nativeInterface.internalSystray;
            root.internalSystray.parent = root;
            root.internalSystray.anchors.fill = root;
        }
    }
}
