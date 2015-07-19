/*
 *   Copyright 2011 Marco Martin <notmart@gmail.com>
 *   Copyright 2014 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kcoreaddons 1.0 as KCoreAddons
import org.kde.kquickcontrolsaddons 2.0

// Unfortunately ColumnLayout was a pain to use, so it's a Column with Rows inside
Column {
    id: detailsItem

    spacing: jobItem.layoutSpacing

    readonly property int eta: jobItem.getData(jobsSource.data, "eta", 0)
    readonly property string speed: jobItem.getData(jobsSource.data, "speed", '')

    property int leftColumnWidth

    function localizeProcessedAmount(id) {
        if (!jobsSource.data[modelData]) {
            return ""
        }

        var unit = jobsSource.data[modelData]["processedUnit" + id]
        var processed = jobsSource.data[modelData]["processedAmount" + id]
        var total = jobsSource.data[modelData]["totalAmount" + id]

        //if bytes localise the unit
        if (unit === "bytes") {
            return i18nc("How much many bytes (or whether unit in the locale has been copied over total", "%1 of %2",
                    KCoreAddons.Format.formatByteSize(processed),
                    KCoreAddons.Format.formatByteSize(total))
        //else print something only if is interesting data (ie more than one file/directory etc to copy
        } else if (total > 1) {
            // HACK Actually the owner of the job is responsible for sending the unit in a user-displayable
            // way but this has been broken for years and every other unit (other than files and dirs) is correct
            if (unit === "files") {
                return i18ncp("Either just 1 file or m of n files are being processed", "1 file", "%2 of %1 files", total, processed)
            } else if (unit === "dirs") {
                return i18ncp("Either just 1 dir or m of n dirs are being processed", "1 dir", "%2 of %1 dirs", total, processed)
            }

            return i18n("%1 of %2 %3", processed, total, unit)
        } else {
            return ""
        }
    }

    // The 2 main labels (eg. Source and Destination)
    Repeater {
        model: 2

        RowLayout {
            width: parent.width
            spacing: jobItem.layoutSpacing
            visible: labelNameText.text !== "" || labelText.text !== ""

            PlasmaComponents.Label {
                id: labelNameText
                Layout.minimumWidth: leftColumnWidth
                Layout.maximumWidth: leftColumnWidth
                height: paintedHeight
                onPaintedWidthChanged: {
                    if (paintedWidth > leftColumnWidth) {
                        leftColumnWidth = paintedWidth
                    }
                }

                font: theme.smallestFont
                text: jobItem["labelName" + index] ? i18nc("placeholder is row description, such as Source or Destination", "%1:", jobItem["labelName" + index]) : ""
                horizontalAlignment: Text.AlignRight
            }

            PlasmaComponents.Label {
                id: labelText
                Layout.fillWidth: true
                height: paintedHeight

                font: theme.smallestFont
                text: jobItem["label" + index] ? jobItem["label" + index] : ""
                elide: Text.ElideMiddle

                PlasmaCore.ToolTipArea {
                    anchors.fill: parent
                    subText: labelText.truncated ? jobItem["label" + index] : ""
                }
            }
        }
    }

    // The three details rows (eg. how many files and folders have been copied and the total amount etc)
    Repeater {
        model: 3

        PlasmaComponents.Label {
            id: detailsLabel
            anchors {
                left: parent.left
                leftMargin: leftColumnWidth + jobItem.layoutSpacing
                right: parent.right
            }
            height: paintedHeight

            text: localizeProcessedAmount(index)
            font: theme.smallestFont
            visible: text !== ""
        }
    }

    PlasmaComponents.Label {
        id: speedLabel
        anchors {
            left: parent.left
            leftMargin: leftColumnWidth + jobItem.layoutSpacing
            right: parent.right
        }
        height: paintedHeight

        font: theme.smallestFont
        text: eta > 0 ? i18nc("Speed and estimated time to completion", "%1 (%2 remaining)", speed, KCoreAddons.Format.formatSpelloutDuration(eta)) : speed
        visible: eta > 0 || parseInt(speed) > 0
    }

}
