/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick

import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid
import org.kde.plasma.extras

AbstractItem {
    id: taskIcon

    itemId: model.itemId
    text: model.name
    mainText: inVisibleLayout ? model.name : ""
    subText: model.message
    textFormat: Text.AutoText
    active: inVisibleLayout || text != mainText || subText.length > 0

    Kirigami.Icon {
        id: iconItem
        parent: taskIcon.iconContainer
        anchors.fill: iconItem.parent

        source: taskIcon.model.icon
        active: taskIcon.containsMouse
    }

    onContextMenu: mouse => {
        if (mouse === null) {
            openContextMenu(Plasmoid.popupPosition(taskIcon, taskIcon.width / 2, taskIcon.height / 2));
        } else {
            openContextMenu(Plasmoid.popupPosition(taskIcon, mouse.x, mouse.y));
        }
    }

    onClicked: mouse => {
        const pos = Plasmoid.popupPosition(taskIcon, mouse.x, mouse.y);
        switch (mouse.button) {
        case Qt.LeftButton:
            taskIcon.activated(pos)
            break;
        case Qt.RightButton:
            openContextMenu(pos);
        }
    }

    function baseModelIndex() {
        let index = parent.GridView.view.model.index(taskIcon.index, 0);
        while (index.model.hasOwnProperty("mapToSource")) {
            index = index.model.mapToSource(index)
        }
        return index;
    }

    function openContextMenu(pos = Qt.point(width/2, height/2)) {
        contextMenu.open(pos)
    }

    onActivated: pos => {
        const index = baseModelIndex();
        index.model.openBackgroundApp(index);
    }

    Menu {
        id: contextMenu
        MenuItem {
            text: i18nc("@action:inmenu", "Quit %1", model.name)
            icon: "application-exit"
            onClicked: {
                const index = baseModelIndex();
                index.model.stopBackgroundApp(index);
            }
        }
    }


}
