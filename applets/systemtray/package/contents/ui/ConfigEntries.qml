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

import QtQuick 2.5
import QtQuick.Controls 1.4 as QQC1
import QtQuick.Controls 2.5 as QQC2
import QtQuick.Layouts 1.3

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kquickcontrols 2.0 as KQC
import org.kde.kirigami 2.5 as Kirigami

ColumnLayout {
    id: iconsPage

    signal configurationChanged

    property var cfg_shownItems: []
    property var cfg_hiddenItems: []
    property alias cfg_showAllItems: showAllCheckBox.checked

    function saveConfig () {
        for (var i in tableView.model) {
            //tableView.model[i].applet.globalShortcut = tableView.model[i].shortcut
        }
    }

    PlasmaCore.DataSource {
        id: statusNotifierSource
        engine: "statusnotifieritem"
        interval: 0
        onSourceAdded: {
            connectSource(source)
        }
        Component.onCompleted: {
            connectedSources = sources
        }
   }

    PlasmaCore.SortFilterModel {
       id: statusNotifierModel
       sourceModel: PlasmaCore.DataModel {
           dataSource: statusNotifierSource
       }
    }


    Kirigami.FormLayout {

        QQC2.CheckBox {
            id: showAllCheckBox
            text: i18n("Always show all entries")
        }

        QQC2.Button { // just for measurement
            id: measureButton
            text: "measureButton"
            visible: false
        }

        // resizeToContents does not take into account the heading
        QQC2.Label {
            id: shortcutColumnMeasureLabel
            text: shortcutColumn.title
            visible: false
        }
    }


    function retrieveAllItems() {
        var list = [];
        for (var i = 0; i < statusNotifierModel.count; ++i) {
            var item = statusNotifierModel.get(i);
            list.push({
                "index": i,
                "taskId": item.Id,
                "name": item.Title,
                "iconName": item.IconName,
                "icon": item.Icon
            });
        }
        var lastIndex = list.length;
        for (var i = 0; i < plasmoid.applets.length; ++i) {
            var item = plasmoid.applets[i]
            list.push({
                "index": (i + lastIndex),
                "applet": item,
                "taskId": item.pluginName,
                "name": item.title,
                "iconName": item.icon,
                "shortcut": item.globalShortcut
            });
        }
        list.sort(function(a, b) {
            return a.name.localeCompare(b.name);
        });
        return list;
    }

    // There is no QQC2 version of TableView yet
    QQC1.TableView {
        id: tableView
        Layout.fillWidth: true
        Layout.fillHeight: true

        model: retrieveAllItems()
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

        QQC1.TableViewColumn {
            id: entryColumn
            width: tableView.viewport.width - visibilityColumn.width - shortcutColumn.width
            title: i18nc("Name of the system tray entry", "Entry")
            movable: false
            resizable: false

            delegate: RowLayout {
                Item { // spacer
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                }

                QIconItem {
                    width: units.iconSizes.small
                    height: width
                    icon: modelData.iconName || modelData.icon || ""
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    text: modelData.name
                    elide: Text.ElideRight
                    wrapMode: Text.NoWrap
                }
            }
        }

        QQC1.TableViewColumn {
            id: visibilityColumn
            title: i18n("Visibility")
            movable: false
            resizable: false

            delegate: QQC2.ComboBox {
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
                        if (shownIndex === -1) {
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
                        if (hiddenIndex === -1) {
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

        QQC1.TableViewColumn {
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
                        if (keySequence !== modelData.shortcut) {
                            // both SNIs and plasmoids are listed in the same TableView
                            // but they come from two separate models, so we need to subtract
                            // the SNI model count to get the actual plasmoid index
                            var index = modelData.index - statusNotifierModel.count
                            plasmoid.applets[index].globalShortcut = keySequence

                            iconsPage.configurationChanged()
                        }

                        shortcutColumn.resizeToContents()
                    }
                }
            }
        }
    }
}
