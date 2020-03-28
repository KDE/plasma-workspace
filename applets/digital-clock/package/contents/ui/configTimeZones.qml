/*
 * Copyright 2013 Kai Uwe Broulik <kde@privat.broulik.de>
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

import QtQuick 2.12
import QtQuick.Controls 2.8 as QQC2
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.1

import org.kde.plasma.private.digitalclock 1.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kirigami 2.5 as Kirigami

ColumnLayout {
    id: timeZonesPage

    property alias cfg_selectedTimeZones: timeZones.selectedTimeZones
    property alias cfg_wheelChangesTimezone: enableWheelCheckBox.checked

    TimeZoneModel {
        id: timeZones

        onSelectedTimeZonesChanged: {
            if (selectedTimeZones.length === 0) {
                messageWidget.visible = true;

                timeZones.selectLocalTimeZone();
            }
        }
    }

    Kirigami.InlineMessage {
        id: messageWidget
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.smallSpacing
        type: Kirigami.MessageType.Warning
        text: i18n("At least one time zone needs to be enabled. 'Local' was enabled automatically.")
        showCloseButton: true
    }

    QQC2.TextField {
        id: filter
        Layout.fillWidth: true
        placeholderText: i18n("Search Time Zones")
    }

    Item {
        Layout.fillWidth: true
        Layout.fillHeight: true

        QQC2.ScrollView {
            anchors.fill: parent
            clip: true
            Component.onCompleted: background.visible = true // enable border

            ListView {
                id: listView
                focus: true // keyboard navigation
                activeFocusOnTab: true // keyboard navigation

                model: TimeZoneFilterProxy {
                    sourceModel: timeZones
                    filterString: filter.text
                }

                delegate: QQC2.CheckDelegate {
                    id: checkbox
                    focus: true // keyboard navigation
                    width: parent.width
                    text: !city || city.indexOf("UTC") === 0 ? comment : comment ? i18n("%1, %2 (%3)", city, region, comment) : i18n("%1, %2", city, region)
                    checked: model.checked
                    onToggled: {
                        model.checked = checkbox.checked
                        listView.currentIndex = index // highlight
                        listView.forceActiveFocus() // keyboard navigation
                    }
                    highlighted: ListView.isCurrentItem
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        QQC2.CheckBox {
            id: enableWheelCheckBox
            text: i18n("Switch time zone with mouse wheel")
        }
    }

}
