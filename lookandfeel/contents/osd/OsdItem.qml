/*
 * Copyright 2014 Martin Klapetek <mklapetek@kde.org>
 * Copyright 2019 Kai Uwe Broulik <kde@broulik.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtra
import QtQuick.Window 2.2

RowLayout {
    property QtObject rootItem

    spacing: units.smallSpacing

    width: Math.max(Math.min(Screen.desktopAvailableWidth / 2, implicitWidth), units.gridUnit * 15)
    height: units.iconSizes.medium

    PlasmaCore.IconItem {
        Layout.preferredWidth: units.iconSizes.medium
        Layout.preferredHeight: units.iconSizes.medium
        source: rootItem.icon
        visible: valid
    }

    PlasmaComponents.ProgressBar {
        id: progressBar
        Layout.fillWidth: true
        visible: rootItem.showingProgress
        minimumValue: 0
        maximumValue: 100
        value: Number(rootItem.osdValue)
    }

    PlasmaExtra.Heading {
        id: label
        Layout.fillWidth: true
        Layout.fillHeight: true
        level: 3
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        textFormat: Text.PlainText
        wrapMode: Text.NoWrap
        elide: Text.ElideRight
        text: !rootItem.showingProgress && rootItem.osdValue ? rootItem.osdValue : ""
        visible: !rootItem.showingProgress
    }
}
