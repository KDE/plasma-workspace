/*
 *  Copyright 2013 Sebastian Kügler <sebas@kde.org>
 *  Copyright 2014 Marco Martin <mart@kde.org>
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
import org.kde.kquickcontrolsaddons 2.0

import org.kde.private.systemtray 2.0 as SystemTray

Item {
    id: iconsPage
    width: childrenRect.width
    height: childrenRect.height
    implicitWidth: pageColumn.implicitWidth
    implicitHeight: pageColumn.implicitHeight

    property var cfg_extraItems: Array()
    property var cfg_shownItems: Array()
    property var cfg_hiddenItems: Array()

    SystemTray.Host {
        id: host
        plasmoidsAllowed: cfg_extraItems
    }

    SystemPalette {
        id: palette
    }

    Column {
        id: pageColumn
        spacing: itemSizeLabel.height / 2
        width: units.gridUnit * 25

        PlasmaExtras.Heading {
            level: 2
            text: i18n("Entries")
            color: palette.text
        }

        Repeater {
            model: plasmoid.rootItem.systrayHost.allTasks

            delegate: Row {
                height: implicitHeight
                width: parent.width
                QtControls.Label {
                    text: modelData.name
                    elide: Text.ElideRight
                    width: (parent.width / 4) * 3
                }
                QtControls.ComboBox {
                    width: parent.width / 4
                    currentIndex: {
                        if (cfg_shownItems.indexOf(modelData.taskId) != -1) {
                            return 1;
                        } else if (cfg_hiddenItems.indexOf(modelData.taskId) != -1) {
                            return 2;
                        } else {
                            return 0;
                        }
                    }
                    onCurrentIndexChanged: {
                        var shownIndex = cfg_shownItems.indexOf(modelData.taskId);
                        var hiddenIndex = cfg_hiddenItems.indexOf(modelData.taskId);

                        switch (currentIndex) {
                        case 0: {
                            if (shownIndex > -1) {
                                cfg_shownItems.splice(shownIndex, 1);
                            }
                            if (hiddenIndex > -1) {
                                cfg_hiddenItems.splice(hiddenIndex, 1);
                            }
                            break;
                        }
                        case 1: {
                            if (shownIndex == -1) {
                                cfg_shownItems.push(modelData.taskId);
                            }
                            if (hiddenIndex > -1) {
                                cfg_hiddenItems.splice(hiddenIndex, 1);
                            }
                            break;
                        }
                        case 2: {
                            if (shownIndex > -1) {
                                cfg_shownItems.splice(shownIndex, 1);
                            }
                            if (hiddenIndex == -1) {
                                cfg_hiddenItems.push(modelData.taskId);
                            }
                            break;
                        }
                        }
                    }
                    model: ListModel {
                        id: cbItems
                        ListElement { text: "Auto"; val: 1 }
                        ListElement { text: "Shown"; val: 2 }
                        ListElement { text: "Hidden"; val: 0 }
                    }
                }
            }
        }
    }
}
