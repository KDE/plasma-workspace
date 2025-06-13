/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import "data.js" as Data
import org.kde.plasma.private.sessions
import org.kde.kirigami as Kirigami

PlasmoidItem {
    id: root

    readonly property int minButtonSize: Kirigami.Units.iconSizes.small

    preferredRepresentation: fullRepresentation
    fullRepresentation: Flow {
        id: lockout

        Layout.minimumWidth: {
            if (Plasmoid.formFactor === PlasmaCore.Types.Vertical) {
                return 0
            } else if (Plasmoid.formFactor === PlasmaCore.Types.Horizontal) {
                return height < minButtonSize * visibleButtons ? height * visibleButtons : height / visibleButtons;
            } else {
                return width > height ? minButtonSize * visibleButtons : minButtonSize
            }
        }
        Layout.minimumHeight: {
            if (Plasmoid.formFactor === PlasmaCore.Types.Vertical) {
                return width >= minButtonSize * visibleButtons ? width / visibleButtons : width * visibleButtons
            } else if (Plasmoid.formFactor === PlasmaCore.Types.Horizontal) {
                return 0
            } else {
                return width > height ? minButtonSize : minButtonSize * visibleButtons
            }
        }

        Layout.preferredWidth: Layout.minimumWidth
        Layout.preferredHeight: Layout.minimumHeight

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
            if ((Plasmoid.formFactor === PlasmaCore.Types.Vertical && width >= minButtonSize * visibleButtons) ||
                (Plasmoid.formFactor === PlasmaCore.Types.Horizontal && height < minButtonSize * visibleButtons) ||
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

            delegate: PlasmaCore.ToolTipArea {
                id: iconDelegate
                visible: Plasmoid.configuration["show_" + modelData.configKey] && (!modelData.hasOwnProperty("requires") || session["can" + modelData.requires])
                width: items.itemWidth
                height: items.itemHeight

                activeFocusOnTab: true
                mainText: modelData.tooltip_mainText
                subText: modelData.tooltip_subText
                textFormat: Text.PlainText

                Accessible.name: iconDelegate.mainText
                Accessible.description: iconDelegate.subText
                Accessible.role: Accessible.Button
                Keys.onPressed: event => {
                    switch (event.key) {
                    case Qt.Key_Space:
                    case Qt.Key_Enter:
                    case Qt.Key_Return:
                    case Qt.Key_Select:
                        performOperation(modelData.operation)
                        break;
                    }
                }

                TapHandler {
                    id: tapHandler
                    onTapped: performOperation(modelData.operation)
                }

                Kirigami.Icon {
                    id: iconButton
                    width: items.iconSize
                    height: items.iconSize
                    anchors.centerIn: parent
                    source: modelData.icon
                    scale: tapHandler.pressed ? 0.9 : 1
                    active: iconDelegate.containsMouse
                }
            }
        }

        function performOperation(operation) {
            session[operation]()
        }
    }
}
