/*
 *   Copyright 2016 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

AbstractItem {
    id: taskIcon

    itemId: Id
    text: Title
    mainText: ToolTipTitle != "" ? ToolTipTitle : Title
    subText: ToolTipSubTitle
    icon: ToolTipIcon != "" ? ToolTipIcon : Icon ? Icon : IconName
    textFormat: Text.AutoText
    category: Category

    status: {
        switch (Status) {
        case "Active":
            return PlasmaCore.Types.ActiveStatus;
        case "NeedsAttention":
            return PlasmaCore.Types.NeedsAttentionStatus;
        //just assume passive
        default:
            return PlasmaCore.Types.PassiveStatus;
        }
    }

    iconItem: iconItem

    PlasmaCore.IconItem {
        id: iconItem
        source: Icon ? Icon : IconName
        width: Math.min(parent.width, parent.height)
        height: width
        active: taskIcon.containsMouse

        anchors {
            left: parent.left
            verticalCenter: parent.verticalCenter
        }
    }

    onContextMenu: {
        openContextMenu(plasmoid.nativeInterface.popupPosition(taskIcon, mouse.x, mouse.y))
    }

    onClicked: {
        var pos = plasmoid.nativeInterface.popupPosition(taskIcon, mouse.x, mouse.y);

        switch (mouse.button) {
        case Qt.LeftButton: {
            var service = statusNotifierSource.serviceForSource(DataEngineSource);
            var operation = service.operationDescription("Activate");
            operation.x = pos.x;
            operation.y = pos.y;
            var job = service.startOperationCall(operation);
            job.finished.connect(function () {
                if (!job.result) {
                    // On error try to invoke the context menu.
                    // Workaround primarily for apps using libappindicator.
                    openContextMenu(pos);
                }
            });
            break;
        }
        case Qt.RightButton:
            openContextMenu(pos);
            break;

        case Qt.MiddleButton:
            var service = statusNotifierSource.serviceForSource(DataEngineSource);
            var operation = service.operationDescription("SecondaryActivate");
            operation.x = pos.x;

            operation.y = pos.y;
            service.startOperationCall(operation);
            break;
        }
    }

    function openContextMenu(pos) {
        var service = statusNotifierSource.serviceForSource(DataEngineSource);
        var operation = service.operationDescription("ContextMenu");
        operation.x = pos.x;
        operation.y = pos.y;

        var job = service.startOperationCall(operation);
        job.finished.connect(function () {
            plasmoid.nativeInterface.showStatusNotifierContextMenu(job, taskIcon);
        });
    }

    onWheel: {
        //don't send activateVertScroll with a delta of 0, some clients seem to break (kmix)
        if (wheel.angleDelta.y !== 0) {
            var service = statusNotifierSource.serviceForSource(DataEngineSource);
            var operation = service.operationDescription("Scroll");
            operation.delta =wheel.angleDelta.y;
            operation.direction = "Vertical";
            service.startOperationCall(operation);
        }
        if (wheel.angleDelta.x !== 0) {
            var service = statusNotifierSource.serviceForSource(DataEngineSource);
            var operation = service.operationDescription("Scroll");
            operation.delta =wheel.angleDelta.x;
            operation.direction = "Horizontal";
            service.startOperationCall(operation);
        }
    }
}
