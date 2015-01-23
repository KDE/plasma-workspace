/*
 *   Copyright 2012-2013 Daniel Nicoletti <dantti12@gmail.com>
 *   Copyright 2013, 2015 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as Components
import org.kde.kquickcontrolsaddons 2.0

FocusScope {
    id: brightnessItem
    width: parent.width
    height: Math.max(pmCheckBox.height, pmLabel.height) + dialog.anchors.topMargin + dialog.anchors.bottomMargin + units.gridUnit / 2 + (pmHintRow.visible ? pmHintRow.height : 0)

    property alias enabled: pmCheckBox.checked

    Components.CheckBox {
        id: pmCheckBox
        anchors {
            left: parent.left
            leftMargin: Math.round(units.gridUnit / 2) + (units.iconSizes.medium - pmCheckBox.width) / 2
        }
        focus: true
        checked: true
        // we don't want to mess with the checked state but still reflect that changing it might not yield the desired result
        opacity: inhibitions.length > 0 ? 0.5 : 1
        Behavior on opacity {
            NumberAnimation { duration: units.longDuration }
        }
    }

    Components.Label {
        id: pmLabel
        anchors {
            verticalCenter: pmCheckBox.verticalCenter
            left: pmCheckBox.right
            leftMargin: units.gridUnit
        }
        height: paintedHeight
        text: i18n("Enable Power Management")
    }

    MouseArea {
        anchors {
            left: pmCheckBox.left
            top: pmCheckBox.top
            bottom: pmCheckBox.bottom
            right: pmLabel.right
        }
        onClicked: {
            pmCheckBox.forceActiveFocus()
            pmCheckBox.checked = !pmCheckBox.checked
        }
    }

    Components.ToolButton {
        anchors {
            right: parent.right
            rightMargin: Math.round(units.gridUnit / 2)
            verticalCenter: pmCheckBox.verticalCenter
        }
        iconSource: "configure"
        onClicked: processRunner.runPowerdevilKCM()
        tooltip: i18n("Configure Power Saving...")
    }

    RowLayout {
        id: pmHintRow
        anchors {
            left: pmLabel.left
            right: parent.right
            rightMargin: units.gridUnit + percentageMeasurementLabel.width
            top: pmCheckBox.bottom
        }
        spacing: units.smallSpacing
        visible: inhibitions.length > 0

        PlasmaCore.IconItem {
            width: units.iconSizes.small
            height: width
            source: inhibitions.length > 0 ? inhibitions[0].Icon || "" : ""
            visible: valid
        }

        Components.Label {
            Layout.fillWidth: true
            text: {
                if (inhibitions.length > 1) {
                    return i18ncp("Some Application and n others are currently suppressing PM",
                                  "%2 and %1 other application are currently suppressing power management",
                                  "%2 and %1 other applications are currently suppressing power management",
                                  inhibitions.length - 1, inhibitions[0].Name) // plural only works on %1
                } else if (inhibitions.length === 1) {
                    if (!inhibitions[0].Reason) {
                        return i18nc("Some Application is suppressing PM",
                                     "%1 is currently suppressing power management.", inhibitions[0].Name)
                    } else {
                        return i18nc("Some Application is suppressing PM: Reason provided by the app",
                                     "%1 is currently suppressing power management: %2", inhibitions[0].Name, inhibitions[0].Reason)
                    }
                } else {
                    return ""
                }
            }
            height: implicitHeight
            font.pointSize: theme.smallestFont.pointSize
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            maximumLineCount: 2
        }
    }
}

