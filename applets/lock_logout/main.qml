/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

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
    property bool initialized: false

    preferredRepresentation: fullRepresentation
    fullRepresentation: Flow {
        id: lockout

        Layout.minimumWidth: {
            if (Plasmoid.formFactor === PlasmaCore.Types.Vertical) {
                return 0
            } else if (Plasmoid.formFactor === PlasmaCore.Types.Horizontal) {
                return height < root.minButtonSize * visibleButtons ? height * visibleButtons : height / visibleButtons;
            } else {
                return width > height ? root.minButtonSize * visibleButtons : root.minButtonSize
            }
        }
        Layout.minimumHeight: {
            if (Plasmoid.formFactor === PlasmaCore.Types.Vertical) {
                return width >= root.minButtonSize * visibleButtons ? width / visibleButtons : width * visibleButtons
            } else if (Plasmoid.formFactor === PlasmaCore.Types.Horizontal) {
                return 0
            } else {
                return width > height ? root.minButtonSize : root.minButtonSize * visibleButtons
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
            if ((Plasmoid.formFactor === PlasmaCore.Types.Vertical && width >= root.minButtonSize * visibleButtons) ||
                (Plasmoid.formFactor === PlasmaCore.Types.Horizontal && height < root.minButtonSize * visibleButtons) ||
                (width > height)) {
                return Flow.LeftToRight // horizontal
            } else {
                return Flow.TopToBottom // vertical
            }
        }

        SessionManagement {
            id: session
        }

        function buildOrderedModel() {
            orderedActionsModel.clear();

            var data = Data.data;

            var actionMap = {};
            for (const item of data) {
                actionMap[item.configKey] = item;
            }

            var order = Plasmoid.configuration.actionsOrder || [];

            if (!order.length) {
                for (const item of data) {
                    order.push(item.configKey);
                }
            }

            for (const key of order) {
                var item = actionMap[key];
                if (!item) {
                    continue;
                }
                orderedActionsModel.append(item);
            }
        }

        Connections {
            target: Plasmoid.configuration

            function onActionsOrderChanged() {
                if (!root.initialized) {
                    // On first addition to containment for every configuration change, ignore it, 
                    // otherwise the model is built twice
                    return;
                }
                lockout.buildOrderedModel();
            }
        }

        Component.onCompleted: function() {
            lockout.buildOrderedModel();
            root.initialized = true;
        }

        Repeater {
            id: items
            property int itemWidth: parent.flow==Flow.LeftToRight ? Math.floor(parent.width/lockout.visibleButtons) : parent.width
            property int itemHeight: parent.flow==Flow.TopToBottom ? Math.floor(parent.height/lockout.visibleButtons) : parent.height
            property int iconSize: Math.min(itemWidth, itemHeight)

            model: ListModel {
                id: orderedActionsModel
            }

            delegate: PlasmaCore.ToolTipArea {
                id: iconDelegate

                required property string configKey
                required property string requires
                required property string tooltip_mainText
                required property string tooltip_subText
                required property string operation
                required property var model // used to access icon because ToopTipArea already has icon property

                visible: Plasmoid.configuration["show_" + configKey] && (requires !== ""|| session["can" + requires])
                width: items.itemWidth
                height: items.itemHeight

                activeFocusOnTab: true
                mainText: tooltip_mainText
                subText: tooltip_subText
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
                        lockout.performOperation(operation)
                        break;
                    }
                }

                TapHandler {
                    id: tapHandler
                    onTapped: lockout.performOperation(iconDelegate.operation)
                }

                Kirigami.Icon {
                    id: iconButton
                    width: items.iconSize
                    height: items.iconSize
                    anchors.centerIn: parent
                    source: iconDelegate.model.icon
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
