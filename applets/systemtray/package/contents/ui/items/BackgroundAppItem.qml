/*
    SPDX-FileCopyrightText: 2025 Jin Liu <m.liu.jin@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick

import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.plasmoid

AbstractItem {
    id: taskIcon

    itemId: model.itemId
    text: model.display
    mainText: model.display
    subText: model.message
    textFormat: Text.AutoText
    active: subText.length > 0

    Kirigami.Icon {
        id: iconItem
        parent: taskIcon.iconContainer
        anchors.fill: iconItem.parent

        source: {
            return model.decoration
        }
        active: taskIcon.containsMouse
    }

    PlasmaExtras.Menu {
        id: contextMenu
        visualParent: taskIcon

        PlasmaExtras.MenuItem {
            text: i18nc("@action:menu Quit background appliction", "Quit")
            icon: "application-exit-symbolic"
            onClicked: {
                Plasmoid.terminateBackgroundApp(model.appId, model.instanceId);
            }
        }
    }

    onClicked: mouse => {
        switch (mouse.button) {
        case Qt.LeftButton:
            Plasmoid.activateBackgroundApp(model.appId, model.instanceId);
            break;
        case Qt.RightButton:
            contextMenu.open(mouse.x, mouse.y);
            break;
        }
    }
}
