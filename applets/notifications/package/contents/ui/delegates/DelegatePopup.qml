/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2024 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.extras as PlasmaExtras
import org.kde.kirigami as Kirigami
import org.kde.kquickcontrolsaddons as KQuickControlsAddons

import org.kde.notificationmanager as NotificationManager
import org.kde.plasma.private.notifications as Notifications

import "../components" as Components


BaseDelegate {
    id: delegateRoot

    Layout.preferredWidth: footerLoader.item?.implicitWidth ?? -1

    body: bodyLabel
    icon: icon
    footer: footerLoader.item
    columns: 3

    Accessible.role: Accessible.Notification

    readonly property int __firstColumn: modelInterface.urgency === NotificationManager.Notifications.CriticalUrgency ? 1 : 0

    PlasmaExtras.PlasmoidHeading {
        id: heading
        topInset: 0
        Layout.fillWidth: true
        Layout.columnSpan: delegateRoot.__firstColumn + 2
        bottomPadding: 0
        // We want the close button borders to touch popup borders
        leftPadding: Layout.mirrored ? -modelInterface.popupLeftPadding : 0
        rightPadding: Layout.mirrored ? 0 : -modelInterface.popupRightPadding

        // HACK PlasmoidHeading is a QQC2 Control which accepts left mouse button by default,
        // which breaks the popup default action mouse handler, cf. QTBUG-89785
        Component.onCompleted: Notifications.InputDisabler.makeTransparentForInput(this)

        contentItem: Components.NotificationHeader {
            modelInterface: delegateRoot.modelInterface
        }
    }

    // Horizontal timeout indicator
    Item {
        Layout.fillWidth: true
        Layout.row: 1
        Layout.columnSpan: delegateRoot.__firstColumn + 2
        // Hug the top, left, and right
        Layout.topMargin: -((delegateRoot.columnSpacing * 2) + height)
        Layout.leftMargin: -delegateRoot.modelInterface.popupLeftPadding
        Layout.rightMargin: -delegateRoot.modelInterface.popupRightPadding
        implicitHeight: 1
        implicitWidth: -1
        visible: !criticalNotificationIndicator.visible

        Rectangle {
            readonly property real completionFraction: delegateRoot.modelInterface.remainingTime / delegateRoot.modelInterface.timeout

            height: parent.height
            width: Math.round(parent.width * completionFraction)
            anchors.left: parent.left

            color: Kirigami.Theme.highlightColor
        }
    }

    Rectangle {
        id: criticalNotificationIndicator
        Layout.fillHeight: true
        Layout.leftMargin: Layout.mirrored ? 0 : -modelInterface.popupLeftPadding
        Layout.rightMargin: Layout.mirrored ? -modelInterface.popupRightPadding : 0
        Layout.topMargin: -delegateRoot.rowSpacing
        Layout.bottomMargin: -delegateRoot.rowSpacing
        Layout.rowSpan: 3

        implicitWidth: 4
        height: parent.height
        color: Kirigami.Theme.neutralTextColor
        visible: modelInterface.urgency === NotificationManager.Notifications.CriticalUrgency
    }

    Components.Summary {
        id: summary
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        Layout.row: 2
        Layout.column: delegateRoot.__firstColumn
        Layout.columnSpan: icon.visible ? 1 : 2
        modelInterface: delegateRoot.modelInterface
    }

    Components.Icon {
        id: icon
        Layout.row: 2
        Layout.column: delegateRoot.__firstColumn + 1
        Layout.rowSpan: 2
        modelInterface: delegateRoot.modelInterface
    }

    KQuickControlsAddons.MouseEventListener {
        Layout.fillWidth: true
        Layout.row: summary.visible ? 3 : 2
        Layout.column: delegateRoot.__firstColumn
        Layout.columnSpan: icon.visible ? 1 : 2
        Layout.maximumHeight: Kirigami.Units.gridUnit * modelInterface.maximumLineCount
        // The body doesn't need to influence the implicit width in any way, this avoids a binding loop
        implicitWidth: -1
        implicitHeight: scroll.implicitHeight
        onClicked: {
            if (modelInterface.hasDefaultAction) {
                modelInterface.defaultActionInvoked();
            }
        }
        PlasmaComponents3.ScrollView {
            id: scroll
            anchors.fill: parent

            // This avoids a binding loop
            PlasmaComponents3.ScrollBar.vertical.visible: modelInterface.maximumLineCount > 0 && bodyLabel.implicitHeight > parent.Layout.maximumHeight
            PlasmaComponents3.ScrollBar.horizontal.visible: false

            Components.Body {
                id: bodyLabel
                width: scroll.contentItem.width
                modelInterface: delegateRoot.modelInterface
            }
        }
    }

    Components.FooterLoader {
        id: footerLoader
        Layout.fillWidth: true
        Layout.row: 4
        Layout.column: delegateRoot.__firstColumn
        Layout.columnSpan: 2
        modelInterface: delegateRoot.modelInterface
        iconContainerItem: icon
    }
}

