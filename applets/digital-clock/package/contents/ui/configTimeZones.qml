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

import QtQuick 2.0
import QtQuick.Controls 1.2 as QtControls
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

    // This is just for getting the column width
    QtControls.CheckBox {
        id: checkbox
        visible: false
    }

    Kirigami.InlineMessage {
        id: messageWidget

        Layout.fillWidth: true

        type: Kirigami.MessageType.Warning
        text: i18n("At least one time zone needs to be enabled. 'Local' was enabled automatically.")

        showCloseButton: true
    }

    QtControls.TextField {
        id: filter
        Layout.fillWidth: true
        placeholderText: i18n("Search Time Zones")
    }

    QtControls.TableView {
        id: timeZoneView

        signal toggleCurrent

        Layout.fillWidth: true
        Layout.fillHeight: true

        Keys.onSpacePressed: toggleCurrent()

        model: TimeZoneFilterProxy {
            sourceModel: timeZones
            filterString: filter.text
        }

        QtControls.TableViewColumn {
            role: "checked"
            width: checkbox.width
            delegate:
                QtControls.CheckBox {
                    id: checkBox
                    anchors.centerIn: parent
                    checked: styleData.value
                    activeFocusOnTab: false // only let the TableView as a whole get focus
                    onClicked: {
                        //needed for model's setData to be called
                        model.checked = checked;
                    }

                    Connections {
                        target: timeZoneView
                        onToggleCurrent: {
                            if (styleData.row === timeZoneView.currentRow) {
                                model.checked = !checkBox.checked
                            }
                        }
                    }
                }

            resizable: false
            movable: false
        }
        QtControls.TableViewColumn {
            role: "city"
            title: i18n("City")
        }
        QtControls.TableViewColumn {
            role: "region"
            title: i18n("Region")
        }
        QtControls.TableViewColumn {
            role: "comment"
            title: i18n("Comment")
        }
    }

    RowLayout {
        Layout.fillWidth: true
        QtControls.CheckBox {
            id: enableWheelCheckBox
            text: i18n("Switch time zone with mouse wheel")
        }
    }
}
