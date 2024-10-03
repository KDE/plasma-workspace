/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2024 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.extras as PlasmaExtras
import org.kde.kirigami as Kirigami

import org.kde.plasma.private.notifications 2.0 as Notifications

import "delegates" as Delegates


Delegates.BaseDelegate {
    id: delegateRoot

    menuOpen: bodyLabel.contextMenu !== null
    headerHeight: heading.height

    PlasmaExtras.PlasmoidHeading {
        id: heading
        topInset: 0
        Layout.fillWidth: true
        Layout.columnSpan: 2
        leftPadding: modelInterface.headingLeftPadding
        rightPadding: modelInterface.headingRightPadding
        bottomPadding: 0
        background.visible: !modelInterface.inHistory

        // HACK PlasmoidHeading is a QQC2 Control which accepts left mouse button by default,
        // which breaks the popup default action mouse handler, cf. QTBUG-89785
        Component.onCompleted: Notifications.InputDisabler.makeTransparentForInput(this)

        contentItem: Delegates.NotificationHeader {
            modelInterface: delegateRoot.modelInterface
        }
    }

    Delegates.Summary {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        Layout.topMargin: 0
        modelInterface: delegateRoot.modelInterface
    }

    Delegates.Icon {
        Layout.rowSpan: 2
        source: modelInterface.icon
        applicationIconSource: modelInterface.applicationIconSource
    }

    Delegates.Body {
        id: bodyLabel
        Layout.row: 2
        Layout.column: 0
        Layout.fillWidth: true
        modelInterface: delegateRoot.modelInterface
    }
}

