/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2020 Konrad Materka <materka@gmail.com>
    SPDX-FileCopyrightText: 2020 Nate Graham <nate@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.1 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.plasmoid 2.0

import "items"
import org.kde.plasma.extras 2.0 as PlasmaExtras

PlasmaComponents3.ScrollView {
    id: hiddenTasksView

    property alias layout: hiddenTasks

    hoverEnabled: true
    onHoveredChanged: if (!hovered) {
        hiddenTasks.currentIndex = -1;
    }
    background: null

    // HACK: workaround for https://bugreports.qt.io/browse/QTBUG-83890
    PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff
    PlasmaComponents3.ScrollBar.vertical.policy: systemTrayState.activeApplet ? PlasmaComponents3.ScrollBar.AlwaysOff : PlasmaComponents3.ScrollBar.AsNeeded

    GridView {
        id: hiddenTasks

        readonly property int minimumRows: 4
        readonly property int minimumColumns: 4

        cellWidth: Math.floor(Math.min(hiddenTasksView.availableWidth, popup.Layout.minimumWidth) / minimumRows)
        cellHeight: Math.floor(popup.Layout.minimumHeight / minimumColumns)

        currentIndex: -1
        highlight: PlasmaExtras.Highlight {}
        highlightMoveDuration: 0

        pixelAligned: true

        readonly property int itemCount: model.count

        //! This is used in order to identify the minimum required label height in order for all
        //! labels to be aligned properly at all items. At the same time this approach does not
        //! enforce labels with 3 lines at all cases so translations that require only one or two
        //! lines will always look consistent with no too much padding
        readonly property int minLabelHeight: {
            var minHeight = 0;

            for(let i in contentItem.children){
                var gridItem = contentItem.children[i];
                if (!gridItem || !gridItem.hasOwnProperty("item") || !gridItem.item.hasOwnProperty("labelHeight")) {
                    continue;
                }

                if (gridItem.item.labelHeight > minHeight) {
                    minHeight = gridItem.item.labelHeight;
                }
            }

            return minHeight;
        }

        model: PlasmaCore.SortFilterModel {
            sourceModel: Plasmoid.nativeInterface.systemTrayModel
            filterRole: "effectiveStatus"
            filterCallback: (source_row, value) => value === PlasmaCore.Types.PassiveStatus
        }
        delegate: ItemLoader {}

        keyNavigationEnabled: true
        activeFocusOnTab: true

        KeyNavigation.up: hiddenTasksView.KeyNavigation.up

        onActiveFocusChanged: {
            if (activeFocus && currentIndex === -1) {
                currentIndex = 0
            } else if (!activeFocus && currentIndex >= 0) {
                currentIndex = -1
            }
        }
    }
}
