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
    property alias cfg_debug: debugCheck.checked
//     property alias cfg_BoolTest: testBoolConfigField.checked

    function indexToSize(ix) {
        var s = 22;
        if (ix < 1) {
            s = 16;
        } else if (ix == 1) {
            s = 22;
        } else if (ix == 2) {
            s = 32;
        } else if (ix == 3) {
            s = 48;
        } else if (ix == 4) {
            s = 64;
        } else if (ix == 5) {
            s = 96;
        } else if (ix == 6) {
            s = 128;
        } else if (ix == 7) {
            s = 192;
        } else if (ix == 8) {
            s = 256;
        }
        return s;
    }

    function sizeToIndex(s) {
        var ix = 0;
        if (s < 16) {
            ix = 0;
        } else if (s <=22) {
            ix = 1;
        } else if (s <=32) {
            ix = 2;
        } else if (s <= 48) {
            ix = 3;
        } else if (s <= 64) {
            ix = 4;
        } else if (s <= 96) {
            ix = 5;
        } else if (s <= 128) {
            ix = 6;
        } else if (s <=192) {
            ix = 7;
        } else if (s <= 256) {
            ix = 8;
        }
        return ix;
    }

    SystemTray.Host {
        id: host
    }

    Column {
        id: pageColumn
        anchors.fill: parent
        spacing: itemSizeLabel.height / 2
        PlasmaExtras.Title {
            text: i18n("SystemTray Settings")
        }
        QtControls.CheckBox {
            id: debugCheck
            text: "Visual Debugging"
        }

        Row {
            width: parent.width
            height: itemSizeSlider.height
            QtControls.Label {
                id: itemSizeLabel
                text: i18n("Icon size:")
                width: parent.width / 4
            }
            QtControls.Slider {
                id: itemSizeSlider
                width: parent.width / 2

                value: sizeToIndex(cfg_itemSize)
                minimumValue: 0
                maximumValue: 8
                stepSize: 1
                tickmarksEnabled: true
                updateValueWhileDragging: true
                onValueChanged: cfg_itemSize = indexToSize(value);

            }
            PlasmaCore.IconItem {
                source: "nepomuk"
                width: cfg_itemSize
                height: width
//                 anchors {
//                     left: itemSizeSlider.right
//                     verticalCenter: itemSizeSlider.verticalCenter
//                 }
            }
        }
        ListView {
            model: host.categories
            width: parent.width
            height: itemSizeLabel.height * 10
            delegate: Row {
                height: implicitHeight
                width: parent.width
                QtControls.CheckBox {
                    id: categoryCheck
                    text: modelData
                }
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
