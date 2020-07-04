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

import QtQuick 2.14
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtra
import QtQuick.Window 2.2

RowLayout {
    property QtObject rootItem

    spacing: units.smallSpacing

    width: Math.max(Math.min(Screen.desktopAvailableWidth / 2, implicitWidth), units.gridUnit * 15)
    height: units.iconSizes.medium

    PlasmaCore.IconItem {
        Layout.leftMargin: units.smallSpacing
        Layout.preferredWidth: units.iconSizes.medium
        Layout.preferredHeight: units.iconSizes.medium
        source: rootItem.icon
        visible: valid
    }

    PlasmaComponents3.ProgressBar {
        id: progressBar
        Layout.fillWidth: true
        // So it never exceeds the minimum popup size
        Layout.preferredWidth: 1
        Layout.rightMargin: units.smallSpacing
        visible: rootItem.showingProgress
        from: 0
        to: rootItem.osdMaxValue
        value: Number(rootItem.osdValue)
    }

    // Get the width of a three-digit number so we can size the label
    // to the maximum width to avoid the progress bad resizing itself
    TextMetrics {
        id: widestLabelSize
        text: i18n("100%")
        font: percentageLabel.font
    }

    // Numerical display of progress bar value
    PlasmaExtra.Heading {
        id: percentageLabel
        Layout.fillHeight: true
        Layout.preferredWidth: widestLabelSize.width
        Layout.rightMargin: units.smallSpacing
        level: 3
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: i18nc("Percentage value", "%1%", progressBar.value)
        visible: rootItem.showingProgress
        // Display a subtle visual indication that the volume might be
        // dangerously high
        // ------------------------------------------------
        // Keep this in sync with the copies in plasma-pa:ListItemBase.qml
        // and plasma-pa:VolumeSlider.qml
        color: {
            if (progressBar.value <= 100) {
                return theme.textColor
            } else if (progressBar.value > 100 && progressBar.value <= 125) {
                return theme.neutralTextColor
            } else {
                return theme.negativeTextColor
            }
        }
    }

    PlasmaExtra.Heading {
        id: label
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.rightMargin: units.smallSpacing
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
