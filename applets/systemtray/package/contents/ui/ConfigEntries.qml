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

import org.kde.private.systemtray 2.0 as SystemTray

QtLayouts.GridLayout {
    id: iconsPage

    signal configurationChanged

    property var cfg_shownItems: []
    property var cfg_hiddenItems: []
    property alias cfg_showAllItems: showAllCheckBox.checked

    columns: 4 // so we can indent the entries below...

    QtControls.CheckBox {
        id: showAllCheckBox
        QtLayouts.Layout.columnSpan: iconsPage.columns
        QtLayouts.Layout.fillWidth: true
        text: i18n("Always show all entries")
    }

    Repeater {
        model: plasmoid.rootItem.systrayHost.allTasks

        QIconItem {
            QtLayouts.Layout.column: 1
            QtLayouts.Layout.row: index + 1
            width: units.iconSizes.small
            height: width
            icon: modelData.iconName || modelData.icon || ""
        }
    }

    Repeater {
        model: plasmoid.rootItem.systrayHost.allTasks

        QtControls.Label {
            QtLayouts.Layout.column: 2
            QtLayouts.Layout.row: index + 1
            QtLayouts.Layout.fillWidth: true

            text: modelData.name
            elide: Text.ElideRight
            wrapMode: Text.NoWrap
        }
    }

    Repeater {
        model: plasmoid.rootItem.systrayHost.allTasks

        QtControls.ComboBox {
            QtLayouts.Layout.minimumWidth: Math.round(units.gridUnit * 6.5) // ComboBox sizing is broken
            QtLayouts.Layout.column: 3
            QtLayouts.Layout.row: index + 1

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
                iconsPage.configurationChanged();
            }
            model: [i18n("Auto"), i18n("Shown"), i18n("Hidden")]
        }
    }

    Item { // keep the list tight
        QtLayouts.Layout.fillHeight: true
    }
}
