/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2024 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras as PlasmaExtras
import org.kde.kirigami as Kirigami

import org.kde.notificationmanager as NotificationManager
import org.kde.plasma.private.notifications 2.0 as Notifications

import "../components" as Components


BaseDelegate {
    id: delegateRoot

    body: bodyLabel
    footer: footerLoader.item
    columns: 3

    Accessible.role: Accessible.Notification

    readonly property int __firstColumn: modelInterface.urgency === NotificationManager.Notifications.CriticalUrgency ? 1 : 0

    PlasmaExtras.PlasmoidHeading {
        id: heading
        topInset: 0
        Layout.fillWidth: true
        Layout.columnSpan: __firstColumn + 2
        bottomPadding: 0
        leftPadding: Layout.mirrored ? -modelInterface.headingLeftPadding : 0
        rightPadding: Layout.mirrored ? 0 : -modelInterface.headingRightPadding

        // HACK PlasmoidHeading is a QQC2 Control which accepts left mouse button by default,
        // which breaks the popup default action mouse handler, cf. QTBUG-89785
        Component.onCompleted: Notifications.InputDisabler.makeTransparentForInput(this)

        contentItem: Components.NotificationHeader {
            modelInterface: delegateRoot.modelInterface
        }
    }

    Rectangle {
        Layout.fillHeight: true
        Layout.leftMargin: Layout.mirrored ? 0 : -modelInterface.headingLeftPadding
        Layout.rightMargin: Layout.mirrored ? -modelInterface.headingRightPadding : 0
        Layout.topMargin: -delegateRoot.rowSpacing
        Layout.bottomMargin: -delegateRoot.rowSpacing
        Layout.rowSpan: 3

        implicitWidth: 4
        height: parent.height
        color: Kirigami.Theme.neutralTextColor
        visible: modelInterface.urgency === NotificationManager.Notifications.CriticalUrgency
    }

    Components.Summary {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        Layout.row: 1
        Layout.column: __firstColumn
        modelInterface: delegateRoot.modelInterface
    }

    Components.Icon {
        id: icon
        Layout.row: 1
        Layout.column: __firstColumn + 1
        Layout.rowSpan: 2
        source: modelInterface.icon
        applicationIconSource: modelInterface.applicationIconSource
    }

    PlasmaComponents3.ScrollView {
        Layout.fillWidth: true
        Layout.row: 2
        Layout.column: __firstColumn
        readonly property real maximumHeight: Kirigami.Units.gridUnit * modelInterface.maximumLineCount
        readonly property bool truncated: modelInterface.maximumLineCount > 0 && bodyLabel.implicitHeight > maximumHeight
        Layout.maximumHeight: truncated ? maximumHeight : implicitHeight
        Components.Body {
            id: bodyLabel
            modelInterface: delegateRoot.modelInterface
        }
    }

    Components.FooterLoader {
        id: footerLoader
        Layout.fillWidth: true
        Layout.row: 3
        Layout.column: __firstColumn
        Layout.columnSpan: 2
        modelInterface: delegateRoot.modelInterface
        iconContainerItem: icon
    }
}

