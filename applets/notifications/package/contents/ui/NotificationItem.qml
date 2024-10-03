/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2024 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami 2.20 as Kirigami

import "global"
import "delegates" as Delegates

ColumnLayout {
    id: notificationItem

    // We don't want the popups to grow too much due to very long labels
    Layout.preferredWidth: Math.max(footerLoader.implicitWidth, Globals.popupWidth)
    Layout.preferredHeight: implicitHeight

    readonly property Delegates.ModelInterface modelInterface: Delegates.ModelInterface {}

    readonly property bool menuOpen: Boolean(bodyLoader.item?.menuOpen)
                                     || Boolean(footerLoader.item?.menuOpen)
    readonly property bool dragging: Boolean(footerLoader.item?.dragging)
    readonly property bool replying: footerLoader.item?.replying ?? false
    readonly property bool hasPendingReply: footerLoader.item?.hasPendingReply ?? false
    readonly property real headerHeight: bodyLoader.item?.headerHeight ?? 0 //FIXME: REMOVE
    readonly property real textPreferredWidth: Kirigami.Units.gridUnit * 18


    spacing: Kirigami.Units.smallSpacing

    Loader {
        id: bodyLoader
        Layout.fillWidth: true
        sourceComponent: {
            if (modelInterface.inGroup) {
                return delegateGroupedComponent;
            } else {
                return delegateComponent;
            }
        }
    }

    Component {
        id: delegateGroupedComponent
        DelegateHistoryGrouped {
            modelInterface: notificationItem.modelInterface
        }
    }
    Component {
        id: delegateComponent
        DelegateHistory {
            modelInterface: notificationItem.modelInterface
        }
    }

    Delegates.FooterLoader {
        id: footerLoader
        Layout.fillWidth: true
        modelInterface: notificationItem.modelInterface
    }
}
