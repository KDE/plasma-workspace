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
import plasma.applet.org.kde.plasma.notifications as Notifications

import "../components" as Components


BaseDelegate {
    id: delegateRoot

    Layout.preferredWidth: (footerLoader.item as Item)?.implicitWidth ?? -1

    body: bodyLabel
    icon: icon
    footer: footerLoader.item as Item
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
        leftPadding: Layout.mirrored ? -delegateRoot.modelInterface.popupLeftPadding : 0
        rightPadding: Layout.mirrored ? 0 : -delegateRoot.modelInterface.popupRightPadding

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
        Layout.topMargin: -((delegateRoot.rowSpacing * 2) + height)
        Layout.leftMargin: -delegateRoot.modelInterface.popupLeftPadding
        Layout.rightMargin: -delegateRoot.modelInterface.popupRightPadding
        implicitHeight: 2
        implicitWidth: -1
        visible: !criticalNotificationIndicator.visible && Notifications.Globals.notificationSettings.showPopupTimeout

        Rectangle {
            readonly property real completionFraction: {
                if (delegateRoot.modelInterface.timeout === 0) {
                    return 0
                }

                return delegateRoot.modelInterface.remainingTime / delegateRoot.modelInterface.timeout
            }

            height: parent.height
            width: Math.round(parent.width * completionFraction)
            anchors.left: parent.left

            color: Kirigami.Theme.highlightColor
        }
    }

    Rectangle {
        id: criticalNotificationIndicator
        Layout.fillHeight: true
        Layout.leftMargin: Layout.mirrored ? 0 : -delegateRoot.modelInterface.popupLeftPadding
        Layout.rightMargin: Layout.mirrored ? -delegateRoot.modelInterface.popupRightPadding : 0
        Layout.topMargin: -delegateRoot.rowSpacing
        Layout.bottomMargin: -delegateRoot.rowSpacing
        Layout.rowSpan: 3

        implicitWidth: 4
        height: parent.height
        color: Kirigami.Theme.neutralTextColor
        visible: delegateRoot.modelInterface.urgency === NotificationManager.Notifications.CriticalUrgency
    }

    Components.Summary {
        id: summary
        // Base layout intentionally has no row spacing, so add top padding here when needed
        Layout.topMargin: delegateRoot.hasBodyText || icon.visible ? Kirigami.Units.smallSpacing : 0
        Layout.fillWidth: true
        Layout.row: 2
        Layout.column: delegateRoot.__firstColumn
        Layout.columnSpan: icon.visible ? 1 : 2
        modelInterface: delegateRoot.modelInterface

        KQuickControlsAddons.MouseEventListener {
            anchors.fill: parent
            visible: delegateRoot.modelInterface.hasDefaultAction && !delegateRoot.hasBodyText
            onClicked: delegateRoot.modelInterface.defaultActionInvoked();
        }
    }

    Components.Icon {
        id: icon
        // Base layout intentionally has no row spacing, so add top padding here
        Layout.topMargin: Kirigami.Units.smallSpacing
        Layout.row: 2
        Layout.column: delegateRoot.__firstColumn + 1
        Layout.rowSpan: 2
        modelInterface: delegateRoot.modelInterface
    }

    KQuickControlsAddons.MouseEventListener {
        // Base layout intentionally has no row spacing, so add top padding here when needed
        Layout.topMargin: summary.visible ? 0 : Kirigami.Units.smallSpacing
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.row: summary.visible ? 3 : 2
        Layout.column: delegateRoot.__firstColumn
        Layout.columnSpan: icon.visible ? 1 : 2
        Layout.maximumHeight: Kirigami.Units.gridUnit * delegateRoot.modelInterface.maximumLineCount
        // The body doesn't need to influence the implicit width in any way, this avoids a binding loop
        implicitWidth: -1
        implicitHeight: scroll.implicitHeight
        visible: delegateRoot.hasBodyText
        onClicked: {
            if (delegateRoot.modelInterface.hasDefaultAction) {
                delegateRoot.modelInterface.defaultActionInvoked();
            }
        }
        PlasmaComponents3.ScrollView {
            id: scroll
            anchors.fill: parent
            contentWidth: bodyLabel.width

            // This avoids a binding loop
            PlasmaComponents3.ScrollBar.vertical.visible: delegateRoot.modelInterface.maximumLineCount > 0 && bodyLabel.implicitHeight > parent.Layout.maximumHeight
            PlasmaComponents3.ScrollBar.horizontal.visible: false

            Components.Body {
                id: bodyLabel
                width: scroll.width - scroll.PlasmaComponents3.ScrollBar.vertical.width
                modelInterface: delegateRoot.modelInterface
                Accessible.ignored: true // ignore HTML body in favor of Accessible.description on delegateRoot
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

