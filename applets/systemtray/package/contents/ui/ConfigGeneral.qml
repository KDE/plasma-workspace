/*
 *  Copyright 2013 Sebastian Kügler <sebas@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
 */

import QtQuick 2.0
import QtQuick.Controls 1.0 as QtControls

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.private.systemtray 2.0 as SystemTray

Item {
    id: iconsPage
    width: childrenRect.width
    height: childrenRect.height
    implicitWidth: mainColumn.implicitWidth
    implicitHeight: pageColumn.implicitHeight

    property int cfg_itemSize: plasmoid.configuration.itemSize
    property var cfg_shownCategories: Array()
    property alias cfg_applicationStatusShown: applicationStatus.checked
    property alias cfg_communicationsShown: communications.checked
    property alias cfg_systemServicesShown: systemServices.checked
    property alias cfg_hardwareControlShown: hardwareControl.checked
    property alias cfg_miscellaneousShown: miscellaneous.checked

    SystemTray.Host {
        id: host
    }

    Column {
        id: pageColumn
        spacing: itemSizeLabel.height / 2

        Column {
            QtControls.CheckBox {
                id: applicationStatus
                text: i18n("Application Status")
            }
            QtControls.CheckBox {
                id: communications
                text: i18n("Communications")
            }
            QtControls.CheckBox {
                id: systemServices
                text: i18n("System Services")
            }
            QtControls.CheckBox {
                id: hardwareControl
                text: i18n("Hardware Control")
            }
            QtControls.CheckBox {
                id: miscellaneous
                text: i18n("Miscellaneous")
            }
        }
        ListView {
            model: host.tasks
            width: parent.width
            height: 400

            spacing: parent.spacing

            delegate: Row {
                height: implicitHeight
                width: parent.width
                QtControls.Label {
                    text: name
                    elide: Text.ElideRight
                    width: parent.width / 3
                }
                QtControls.ComboBox {
                    width: 200
                    currentIndex: 0
                    model: ListModel {
                        id: cbItems
                        ListElement { text: "Auto"; val: 1 }
                        ListElement { text: "Shown"; val: 2 }
                        ListElement { text: "Hidden"; val: 0 }
                    }
                    onCurrentIndexChanged: {
                        if (index == 0) {
                            print(name + "Now Hidden")
                        } else if (index == 1) {
                            print(name + "Now Auto")
                        } else {
                            print(name + "Now Shown")
                        }
                        console.debug(cbItems.get(currentIndex).text + ", " + cbItems.get(currentIndex).val)
                    }
                }
            }
        }
    }
}
