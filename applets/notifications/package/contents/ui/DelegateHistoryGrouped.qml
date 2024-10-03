/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2024 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

import "delegates" as Delegates


Delegates.BaseDelegate {
    id: delegateRoot

    menuOpen: bodyLabel.contextMenu !== null

    RowLayout {
        id: summaryRow

        Layout.fillWidth: true
        Layout.columnSpan: 2
        Layout.alignment: Qt.AlignTop

        Delegates.Summary {
            id: summaryLabel

            Layout.topMargin: lineCount > 1 ? Math.max(0, (headingElement.implicitHeight - metrics.height) / 2) : 0

            modelInterface: delegateRoot.modelInterface
        }

        Delegates.NotificationHeader {
            id: headingElement
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            modelInterface: delegateRoot.modelInterface
        }
    }

    Delegates.Body {
        id: bodyLabel
        Layout.fillWidth: true
        modelInterface: delegateRoot.modelInterface
    }

    Delegates.Icon {
        Layout.row: 1
        Layout.column: 1
        source: modelInterface.icon
        applicationIconSource: modelInterface.applicationIconSource
    }
}

