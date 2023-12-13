/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.1
import QtQuick.Layouts 1.1
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid 2.0

//SystemTray is a Containment. To have it presented as a widget in Plasma we need thin wrapping applet
PlasmoidItem {
    id: root

    property ContainmentItem internalSystray

    Layout.minimumWidth: internalSystray ? internalSystray.Layout.minimumWidth : 0
    Layout.minimumHeight: internalSystray ? internalSystray.Layout.minimumHeight : 0
    Layout.preferredWidth: Layout.minimumWidth
    Layout.preferredHeight: Layout.minimumHeight
    Layout.maximumWidth: Layout.minimumWidth
    Layout.maximumHeight: Layout.minimumHeight

    preferredRepresentation: fullRepresentation
    Plasmoid.status: internalSystray ? internalSystray.plasmoid.status : PlasmaCore.Types.UnknownStatus

    //synchronize state between SystemTray and wrapping Applet
    onExpandedChanged: {
        if (internalSystray) {
            internalSystray.expanded = root.expanded
        }
    }
    Connections {
        target: internalSystray
        function onExpandedChanged() {
            root.expanded = internalSystray.expanded
        }
    }

    Component.onCompleted: {
        root.internalSystray = Plasmoid.internalSystray;

        if (root.internalSystray == null) {
            return;
        }
        root.internalSystray.parent = root;
        root.internalSystray.anchors.fill = root;
    }

    Connections {
        target: Plasmoid
        function onInternalSystrayChanged() {
            root.internalSystray = Plasmoid.internalSystray;
            root.internalSystray.parent = root;
            root.internalSystray.anchors.fill = root;
        }
    }
}
