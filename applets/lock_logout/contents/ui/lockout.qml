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
import org.kde.kquickcontrolsaddons 2.0
import "data.js" as Data
import org.kde.plasma.private.sessions 2.0

Flow {
    id: lockout
    Layout.minimumWidth: {
        if (plasmoid.formFactor === PlasmaCore.Types.Vertical) {
            return 0
        } else if (plasmoid.formFactor === PlasmaCore.Types.Horizontal) {
            return height < minButtonSize * visibleButtons ? height * visibleButtons : height / visibleButtons - 1;
        } else {
            return width > height ? minButtonSize * visibleButtons : minButtonSize
        }
    }
    Layout.minimumHeight: {
        if (plasmoid.formFactor === PlasmaCore.Types.Vertical) {
            return width >= minButtonSize * visibleButtons ? width / visibleButtons - 1 : width * visibleButtons
        } else if (plasmoid.formFactor === PlasmaCore.Types.Horizontal) {
            return 0
        } else {
            return width > height ? minButtonSize : minButtonSize * visibleButtons
        }
    }

    Layout.preferredWidth: Layout.minimumWidth
    Layout.preferredHeight: Layout.minimumHeight

    readonly property int minButtonSize: units.iconSizes.small

    Plasmoid.preferredRepresentation: Plasmoid.fullRepresentation
    readonly property int visibleButtons: {
        var count = 0
        for (var i = 0, j = items.count; i < j; ++i) {
            if (items.itemAt(i).visible) {
                ++count
            }
        }
        return count
    }

    flow: {
        if ((plasmoid.formFactor === PlasmaCore.Types.Vertical && width >= minButtonSize * visibleButtons) ||
            (plasmoid.formFactor === PlasmaCore.Types.Horizontal && height < minButtonSize * visibleButtons) ||
            (width > height)) {
            return Flow.LeftToRight // horizontal
        } else {
            return Flow.TopToBottom // vertical
        }
    }

    SessionManagement {
        id: session
    }

    Repeater {
        id: items
        property int itemWidth: parent.flow==Flow.LeftToRight ? Math.floor(parent.width/visibleButtons) : parent.width
        property int itemHeight: parent.flow==Flow.TopToBottom ? Math.floor(parent.height/visibleButtons) : parent.height
        property int iconSize: Math.min(itemWidth, itemHeight)

        model: Data.data

        delegate: Item {
            id: iconDelegate
            visible: plasmoid.configuration["show_" + modelData.configKey] && (!modelData.hasOwnProperty("requires") || session["can" + modelData.requires])
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
                    onReleased: clickHandler(modelData.operation, this)

                    PlasmaCore.ToolTipArea {
                        anchors.fill: parent
                        mainText: modelData.tooltip_mainText
                        subText: modelData.tooltip_subText
                    }
                }
            } 
        }
    }

    function clickHandler(what, button) {
        performOperation(what);
    }

    function performOperation(operation) {
        session[operation]()
    }
}

