/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2020 Konrad Materka <materka@gmail.com>
    SPDX-FileCopyrightText: 2020 Nate Graham <nate@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts

import org.kde.kitemmodels as KItemModels
import org.kde.kirigami as Kirigami
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

        readonly property int columns: 2
        readonly property int minimumColumns: 4

        // Keep these in sync with ItemLoader.qml
        readonly property int delegateMaxTextLines: 2
        readonly property int delegateMargins: Kirigami.Units.smallSpacing

        readonly property int delegateHeight: (metrics.height * delegateMaxTextLines) + (delegateMargins * 2)

        TextMetrics {
            id: metrics
            text: i18nc("Some letters with tall characters, ascenders, descenders, etc", "AILlmyjgGJP")
        }

        cellWidth: Math.floor(hiddenTasksView.availableWidth / columns)
        cellHeight: delegateHeight

        currentIndex: -1
        highlight: PlasmaExtras.Highlight {}
        highlightMoveDuration: 0

        pixelAligned: true

        readonly property int itemCount: model.count

        model: root.hiddenModel
        delegate: Items.ItemLoader {
            width: hiddenTasks.cellWidth
            height: hiddenTasks.cellHeight
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
