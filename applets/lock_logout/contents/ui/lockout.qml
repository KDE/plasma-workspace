/*
 *   Copyright 2011 Viranch Mehta <viranch.mehta@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0
import QtQuick.Layouts 1.0
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0
import org.kde.kquickcontrolsaddons 2.0
import "data.js" as Data

Flow {
    id: lockout
    Layout.minimumWidth: plasmoid.formFactor == PlasmaCore.Types.Vertical ? 0 : height * visibleButtons
    Layout.minimumHeight: plasmoid.formFactor == PlasmaCore.Types.Vertical ? width * visibleButtons : 0

    property int minButtonSize: 16

    Plasmoid.preferredRepresentation: Plasmoid.fullRepresentation
    property int visibleButtons: plasmoid.configuration.show_lockScreen + plasmoid.configuration.show_switchUser + plasmoid.configuration.show_requestShutDown + plasmoid.configuration.show_suspendToRam + plasmoid.configuration.show_suspendToDisk;

    property int orientation: Qt.Vertical

    flow: orientation == Qt.Vertical ? Flow.TopToBottom : Flow.LeftToRight

    onWidthChanged: checkLayout();
    onHeightChanged: checkLayout();

    PlasmaCore.DataSource {
        id: dataEngine
        engine: "powermanagement"
        connectedSources: ["PowerDevil"]
    }

    function checkLayout() {
        switch(plasmoid.formFactor) {
        case PlasmaCore.Types.Vertical:
            if (width >= minButtonSize*visibleButtons) {
                orientation = Qt.Horizontal;
                lockout.Layout.minimumHeight = width/visibleButtons - 1;
                plasmoid.setPreferredSize(width, width/visibleButtons);
            } else {
                orientation = Qt.Vertical;
                lockout.Layout.minimumHeight = width*visibleButtons;
                plasmoid.setPreferredSize(width, width*visibleButtons);
            }
            break;

        case PlasmaCore.Types.Horizontal:
            if (height < minButtonSize*visibleButtons) {
                orientation = Qt.Horizontal;
                lockout.Layout.minimumWidth = height*visibleButtons;
                plasmoid.setPreferredSize(height*visibleButtons, height);
            } else {
                orientation = Qt.Vertical;
                lockout.Layout.minimumWidth = height/visibleButtons - 1;
                plasmoid.setPreferredSize(height/visibleButtons, height);
            }
            break;

        default:
            if (width > height) {
                orientation = Qt.Horizontal;
                lockout.Layout.minimumWidth = minButtonSize*visibleButtons;
                lockout.Layout.minimumHeight = minButtonSize;
            } else {
                orientation = Qt.Vertical;
                lockout.Layout.minimumWidth = minButtonSize;
                lockout.Layout.minimumHeight = minButtonSize*visibleButtons;
            }
            break;
        }
    }


    Repeater {
        id: items
        property int itemWidth: parent.flow==Flow.LeftToRight ? Math.floor(parent.width/visibleButtons) : parent.width
        property int itemHeight: parent.flow==Flow.TopToBottom ? Math.floor(parent.height/visibleButtons) : parent.height
        property int iconSize: Math.min(itemWidth, itemHeight)

        model: Data.data

        delegate: Item {
            id: iconDelegate
            visible: plasmoid.configuration["show_"+modelData.operation]
            width: items.itemWidth
            height: items.itemHeight

            PlasmaCore.IconItem {
                id: iconButton
                width: items.iconSize
                height: items.iconSize
                anchors.centerIn: parent
                source: modelData.icon
                scale: mouseArea.pressed ? 0.9 : 1
                active: mouseArea.containsMouse

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onReleased: clickHandler(modelData.operation)

                    PlasmaCore.ToolTipArea {
                        anchors.fill: parent
                        mainText: modelData.tooltip_mainText
                        subText: modelData.tooltip_subText
                        icon: modelData.icon
                    }
                }
            } 
        }
    }

    Component {
        id: hibernateDialogComponent
        QueryDialog {
            titleIcon: "system-suspend-hibernate"
            titleText: i18n("Hibernate")
            message: i18n("Do you want to suspend to disk (hibernate)?")

            acceptButtonText: i18n("Yes")
            rejectButtonText: i18n("No")

            onAccepted: performOperation("suspendToDisk")
        }
    }
    property QueryDialog hibernateDialog

    Component {
        id: sleepDialogComponent
        QueryDialog {
            titleIcon: "system-suspend"
            titleText: i18n("Suspend")
            message: i18n("Do you want to suspend to RAM (sleep)?")

            acceptButtonText: i18n("Yes")
            rejectButtonText: i18n("No")

            onAccepted: performOperation("suspendToRam")
        }
    }
    property QueryDialog sleepDialog

    function clickHandler(what) {
        if (what == "suspendToDisk") {
            if (!hibernateDialog) {
                hibernateDialog = hibernateDialogComponent.createObject(lockout);
            }

            hibernateDialog.open();

        } else if (what == "suspendToRam") {
            if (!sleepDialog) {
                sleepDialog = sleepDialogComponent.createObject(lockout);
            }

            sleepDialog.open();

        } else {
            performOperation(what);
        }
    }

    function performOperation(what) {
        var service = dataEngine.serviceForSource("PowerDevil");
        var operation = service.operationDescription(what);
        service.startOperationCall(operation);
    }
}

