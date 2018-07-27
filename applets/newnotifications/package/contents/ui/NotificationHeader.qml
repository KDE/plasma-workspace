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

// TODO turn into MouseArea or MEL for expand/collapse
RowLayout {
    id: notificationItem

    property alias icon: iconItem.source
    property alias text: label.text
    property alias ageText: ageLabel.text
    property alias closable: closeButton.visible
    // TODO property bool/alias configurable

    signal closeClicked

    spacing: units.smallSpacing

    PlasmaCore.IconItem {
        id: iconItem
        Layout.preferredWidth: units.iconSizes.small
        Layout.preferredHeight: units.iconSizes.small
    }

    PlasmaExtras.DescriptiveLabel {
        id: label
        Layout.fillWidth: true
        textFormat: Text.PlainText
        // font.pointSize: theme.smallestFont?
    }

    // TODO number of notifications in group, expand collapse arrow

    PlasmaExtras.DescriptiveLabel {
        id: ageLabel
    }

    PlasmaComponents.ToolButton {
        id: closeButton
        iconName: "window-close" // FIXME
        visible: false
    }
}
