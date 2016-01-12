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
import QtQuick.Layouts 1.1 as QtLayouts

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kquickcontrols 2.0 as KQC

import org.kde.private.systemtray 2.0 as SystemTray

QtLayouts.GridLayout {
    id: iconsPage

    signal configurationChanged

    property var cfg_shownItems: []
    property var cfg_hiddenItems: []
    property alias cfg_showAllItems: showAllCheckBox.checked

    columns: 2 // so we can indent the entries below...

    QtControls.CheckBox {
        id: showAllCheckBox
        QtLayouts.Layout.fillWidth: true
        QtLayouts.Layout.columnSpan: iconsPage.columns
        QtLayouts.Layout.row: 1
        text: i18n("Always show all entries")
    }

    QtControls.Button { // just for measurement
        id: measureButton
        visible: false
    }

    // resizeToContents does not take into account the heading
    QtControls.Label {
        id: shortcutColumnMeasureLabel
        text: shortcutColumn.title
        visible: false
    }

    QtControls.TableView {
        id: tableView
        QtLayouts.Layout.fillWidth: true
        QtLayouts.Layout.fillHeight: true
        QtLayouts.Layout.row: 2
        QtLayouts.Layout.column: 1

        model: plasmoid.rootItem.systrayHost.allTasks
        horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff
        flickableItem.boundsBehavior: Flickable.StopAtBounds

        Component.onCompleted: {
            visibilityColumn.resizeToContents()
            shortcutColumn.resizeToContents()
        }

        // Taken from QtQuickControls BasicTableViewStyle, just to make its height sensible...
        rowDelegate: BorderImage {
            visible: styleData.selected || styleData.alternate
            source: "image://__tablerow/" + (styleData.alternate ? "alternate_" : "")
                    + (tableView.activeFocus ? "active" : "")
            height: measureButton.height
            border.left: 4 ; border.right: 4
        }

        QtControls.TableViewColumn {
            id: entryColumn
            width: tableView.width - visibilityColumn.width - shortcutColumn.width - 5
            title: i18nc("Name of the system tray entry", "Entry")
            movable: false
            resizable: false

            delegate: QtLayouts.RowLayout {
                Item { // spacer
                    QtLayouts.Layout.preferredWidth: 1
                    QtLayouts.Layout.fillHeight: true
                }

                QIconItem {
                    width: units.iconSizes.small
                    height: width
                    icon: modelData.iconName || modelData.icon || ""
                }

                QtControls.Label {
                    QtLayouts.Layout.fillWidth: true
                    text: modelData.name
                    elide: Text.ElideRight
                    wrapMode: Text.NoWrap
                }
            }
        }

        QtControls.TableViewColumn {
            id: visibilityColumn
            title: i18n("Visibility")
            movable: false
            resizable: false

            delegate: QtControls.ComboBox {
                implicitWidth: Math.round(units.gridUnit * 6.5) // ComboBox sizing is broken

                enabled: !showAllCheckBox.checked
                currentIndex: {
                    if (cfg_shownItems.indexOf(modelData.taskId) != -1) {
                        return 1;
                    } else if (cfg_hiddenItems.indexOf(modelData.taskId) != -1) {
                        return 2;
                    } else {
                        return 0;
                    }
                }

                // activated, in contrast to currentIndexChanged, only fires if the user himself changed the value
                onActivated: {
                    var shownIndex = cfg_shownItems.indexOf(modelData.taskId);
                    var hiddenIndex = cfg_hiddenItems.indexOf(modelData.taskId);

                    switch (index) {
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
                    iconsPage.configurationChanged();
                }
                model: [i18n("Auto"), i18n("Shown"), i18n("Hidden")]
            }
        }

        QtControls.TableViewColumn {
            id: shortcutColumn
            title: i18n("Keyboard Shortcut") // FIXME doesn't fit
            movable: false
            resizable: false

            // this Item wrapper prevents TableView from ripping apart the two KeySequenceItem buttons
            delegate: Item {
                implicitWidth: Math.max(shortcutColumnMeasureLabel.width, keySequenceItem.width) + 10
                height: keySequenceItem.height

                KQC.KeySequenceItem {
                    id: keySequenceItem
                    anchors.right: parent.right

                    keySequence: modelData.shortcut
                    // only Plasmoids have that
                    visible: modelData.hasOwnProperty("shortcut")
                    onKeySequenceChanged: {
                        if (keySequence != modelData.shortcut) {
                            modelData.shortcut = keySequence
                            iconsPage.configurationChanged()
                        }
                    }
                }
            }
        }
    }
}
