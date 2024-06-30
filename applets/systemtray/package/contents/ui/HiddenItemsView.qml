/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2020 Konrad Materka <materka@gmail.com>
    SPDX-FileCopyrightText: 2020 Nate Graham <nate@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts

import org.kde.kitemmodels as KItemModels
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.plasmoid

import "items" as Items

PlasmaComponents3.ScrollView {
    id: hiddenTasksView

    property alias layout: hiddenTasks

    hoverEnabled: true
    background: null

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
        readonly property int minLabelHeight: Math.max(...contentItem.children.map(gridItem =>
            ((gridItem as Loader)?.item as Items.AbstractItem)?.labelHeight ?? 0
        ));

        model: root.hiddenModel
        delegate: Items.ItemLoader {
            width: hiddenTasks.cellWidth
            height: hiddenTasks.cellHeight
            minLabelHeight: hiddenTasks.minLabelHeight
        }

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
