/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick

import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid

AbstractItem {
    id: taskIcon

    itemId: model.Id
    text: model.Title || model.ToolTipTitle
    mainText: inVisibleLayout || (model.ToolTipTitle !== "" && model.ToolTipTitle !== text) ? model.ToolTipTitle : ""
    subText: model.ToolTipSubTitle
    textFormat: Text.AutoText
    active: inVisibleLayout || text != mainText || subText.length > 0

    Kirigami.Icon {
        id: iconItem
        parent: taskIcon.iconContainer
        anchors.fill: iconItem.parent

        source: {
            if (model.status === PlasmaCore.Types.NeedsAttentionStatus) {
                if (model.AttentionIcon) {
                    return model.AttentionIcon
                }
                if (model.AttentionIconName) {
                    return model.AttentionIconName
                }
            }
            return model.Icon || model.IconName
        }
        active: taskIcon.containsMouse
    }

    onActivated: pos => {
        if (model.ItemIsMenu) {
            openContextMenu(pos);
            return;
        }

        const service = model.Service;
        const operation = service.operationDescription("Activate");
        operation.x = pos.x; //mouseX
        operation.y = pos.y; //mouseY
        const job = service.startOperationCall(operation);
        job.finished.connect(() => {
            if (!job.result) {
                // On error try to invoke the context menu.
                // Workaround primarily for apps using libappindicator.
                openContextMenu(pos);
            }
        })
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
            break;
        case Qt.MiddleButton:
            const service = model.Service;
            const operation = service.operationDescription("SecondaryActivate");
            operation.x = pos.x;
            operation.y = pos.y;
            service.startOperationCall(operation);
            break;
        }
    }

    function openContextMenu(pos = Qt.point(width/2, height/2)) {
        const service = model.Service;
        const operation = service.operationDescription("ContextMenu");
        operation.x = pos.x;
        operation.y = pos.y;

        const job = service.startOperationCall(operation);
        job.finished.connect(() => {
            Plasmoid.showStatusNotifierContextMenu(job, taskIcon);
        });
    }

    onWheel: wheel => {
        //don't send activateVertScroll with a delta of 0, some clients seem to break (kmix)
        if (wheel.angleDelta.y !== 0) {
            const service = model.Service;
            const operation = service.operationDescription("Scroll");
            operation.delta = wheel.angleDelta.y;
            operation.direction = "Vertical";
            service.startOperationCall(operation);
        }
        if (wheel.angleDelta.x !== 0) {
            const service = model.Service;
            const operation = service.operationDescription("Scroll");
            operation.delta = wheel.angleDelta.x;
            operation.direction = "Horizontal";
            service.startOperationCall(operation);
        }
    }
}
