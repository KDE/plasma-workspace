/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore

AbstractItem {
    id: taskIcon

    itemId: model.Id
    text: model.Title || model.ToolTipTitle
    mainText: model.ToolTipTitle !== "" ? model.ToolTipTitle : model.Title
    subText: model.ToolTipSubTitle
    textFormat: Text.AutoText

    PlasmaCore.IconItem {
        id: iconItem
        parent: taskIcon.iconContainer
        anchors.fill: iconItem.parent

        loader: model.IconLoader
        source: {
            if (model.status === PlasmaCore.Types.NeedsAttentionStatus) {
                if (model.AttentionIconName) {
                    return model.AttentionIconName;
                } else if (model.AttentionIconPixmap) {
                    return model.AttentionIconPixmap;
                }
            }
            return model.IconName ? model.IconName : model.IconPixmap
        }
        active: taskIcon.containsMouse
    }

    // IconItem.overlays only supports names so we need a second item for the overlay, using the same
    // positioning that KIconLoader::drawOverlays uses that IconItem uses internally
    PlasmaCore.IconItem {
        id: overlayIconItem
        parent: iconItem.parent

        anchors {
            right: iconItem.right
            bottom: iconItem.bottom
            bottomMargin: Math.floor(0.05 * iconItem.width)
            rightMargin: anchors.bottomMargin
        }
        width: {
            if (iconItem.width < units.iconSizes.medium) {
                return units.iconSizes.small / 2;
            }
            if (iconItem.width <=  units.iconSizes.large) {
                return  units.iconSizes.small
            }
            if (iconItem.width <= 96) {
                return  units.iconSizes.smallMedium
            }
            if (iconItem.width < 256) {
                return  units.iconSizes.medium
            }
            return  units.iconSizes.large;
        }
        height: width

        loader: model.IconLoader
        source: {
            if (model.OverlayIconName) {
                return model.OverlayIconName
            }
            if (model.OverlayIconPixmap) {
                return model.OverlayIconPixmap
            }
            return "";
        }
    }

    onActivated: {
        let service = model.Service;
        let operation = service.operationDescription("Activate");
        operation.x = pos.x; //mouseX
        operation.y = pos.y; //mouseY
        let job = service.startOperationCall(operation);
        job.finished.connect(() => {
            if (!job.result) {
                // On error try to invoke the context menu.
                // Workaround primarily for apps using libappindicator.
                openContextMenu(pos);
            }
        })
        taskIcon.startActivatedAnimation();
    }

    onContextMenu: {
        openContextMenu(Plasmoid.nativeInterface.popupPosition(taskIcon, mouse.x, mouse.y))
    }

    onClicked: {
        var pos = Plasmoid.nativeInterface.popupPosition(taskIcon, mouse.x, mouse.y);
        switch (mouse.button) {
        case Qt.LeftButton:
            taskIcon.activated(pos)
            break;
        case Qt.RightButton:
            openContextMenu(pos);
            break;
        case Qt.MiddleButton:
            var operation = service.operationDescription("SecondaryActivate");
            let service = model.Service;
            operation.x = pos.x;

            operation.y = pos.y;
            service.startOperationCall(operation);
            taskIcon.startActivatedAnimation()
            break;
        }
    }

    function openContextMenu(pos = Qt.point(width/2, height/2)) {
        var service = model.Service;
        var operation = service.operationDescription("ContextMenu");
        operation.x = pos.x;
        operation.y = pos.y;

        var job = service.startOperationCall(operation);
        job.finished.connect(function () {
            Plasmoid.nativeInterface.showStatusNotifierContextMenu(job, taskIcon);
        });
    }

    onWheel: {
        //don't send activateVertScroll with a delta of 0, some clients seem to break (kmix)
        if (wheel.angleDelta.y !== 0) {
            var service = model.Service;
            var operation = service.operationDescription("Scroll");
            operation.delta =wheel.angleDelta.y;
            operation.direction = "Vertical";
            service.startOperationCall(operation);
        }
        if (wheel.angleDelta.x !== 0) {
            var service = model.Service;
            var operation = service.operationDescription("Scroll");
            operation.delta =wheel.angleDelta.x;
            operation.direction = "Horizontal";
            service.startOperationCall(operation);
        }
    }
}
