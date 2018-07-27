/*
 * Copyright 2018 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

import QtQuick 2.8
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.notificationmanager 1.0 as NotificationManager

// TODO turn into MouseArea or MEL for "default action"
// or should that be done where the Popup/ListView is?
ColumnLayout {
    id: notificationItem

    property alias summary: summaryLabel.text
    property alias ageText: ageLabel.text
    property alias closable: closeButton.visible
    // TODO property bool/alias configurable
    property alias body: bodyLabel.text
    property alias icon: iconItem.source

    property var actionNames: []
    property var actionLabels: []

    signal closeClicked
    signal actionInvoked(string actionName)

    spacing: 0

    RowLayout {
        Layout.fillWidth: true
        spacing: units.smallSpacing

        PlasmaExtras.Heading {
            id: summaryLabel
            Layout.fillWidth: true
            textFormat: Text.PlainText
            wrapMode: Text.NoWrap
            elide: Text.ElideRight
            level: 4
        }

        PlasmaExtras.DescriptiveLabel {
            id: ageLabel
        }

        PlasmaComponents.ToolButton {
            id: closeButton
            iconName: "window-close" // FIXME
            visible: false
            onClicked: notificationItem.closeClicked()
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: units.smallSpacing

        // FIXME make selectable, scrollable, links clickable, etc
        PlasmaComponents.Label {
            id: bodyLabel
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: true
            maximumLineCount: 10
            textFormat: Text.StyledText
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
        }

        PlasmaCore.IconItem {
            id: iconItem
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: units.iconSizes.large
            Layout.preferredHeight: units.iconSizes.large
            usesPlasmaTheme: false
            visible: valid
        }
    }

    RowLayout {
        Layout.fillWidth: true
        visible: actionRepeater.count > 0
        spacing: units.smallSpacing

        Repeater {
            id: actionRepeater
            model: notificationItem.actionNames

            PlasmaComponents.ToolButton {
                text: notificationItem.actionLabels[index]
                onClicked: notificationItem.actionInvoked(modelData)
            }
        }

        Item { // compact layout
            Layout.fillWidth: true
        }
    }
}
