/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2015 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2025 Shubham Arora <contact@shubhamarora.dev>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.private.sessions
import org.kde.kcmutils as KCM
import "data.js" as Data

KCM.ScrollViewKCM {
    id: root

    property bool cfg_show_requestLogoutScreen
    property bool cfg_show_requestLogout
    property bool cfg_show_requestShutDown
    property bool cfg_show_requestReboot
    property bool cfg_show_lockScreen
    property bool cfg_show_switchUser
    property bool cfg_show_suspendToDisk
    property bool cfg_show_suspendToRam
    property var cfg_actionsOrder: []
    property list<string> cfgKeys: []

    readonly property int checkedOptions: (Number(cfg_show_requestLogout) +
                                          Number(cfg_show_requestLogoutScreen) +
                                          Number(cfg_show_requestShutDown) +
                                          Number(cfg_show_requestReboot) +
                                          Number(cfg_show_lockScreen) +
                                          Number(cfg_show_switchUser) +
                                          Number(cfg_show_suspendToDisk) +
                                          Number(cfg_show_suspendToRam))

    SessionManagement {
        id: session
    }

    view: ListView {
        id: list
        clip: true
        
        model: ListModel {
            id: actionsModel
        }

        delegate: Loader {
            id: delegateLoader

            required property string enabledKey
            required property string cfgKey
            required property string icon
            required property string text

            width: list.width

            sourceComponent: Kirigami.SwipeListItem {
                id: listItem
                width: list.width

                Kirigami.Theme.useAlternateBackgroundColor: true

                highlighted: false
                hoverEnabled: false
                down: false

                contentItem: RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.ListItemDragHandle {
                        listItem: listItem
                        listView: list
                        onMoveRequested: (oldIndex, newIndex) => {
                            if (oldIndex === newIndex || oldIndex < 0 || newIndex < 0) {
                                return;
                            }

                            actionsModel.move(oldIndex, newIndex, 1);

                            var order = [];
                            for (var i = 0; i < actionsModel.count; ++i) {
                                var model = actionsModel.get(i);
                                if (model && model.configKey) {
                                    order.push(model.configKey);
                                }
                            }
                            root.cfg_actionsOrder = order;
                        }
                    }

                    QQC2.CheckBox {
                        visible: (delegateLoader.enabledKey ? session[delegateLoader.enabledKey] : true)
                        checked: root[delegateLoader.cfgKey]
                        onToggled: root[delegateLoader.cfgKey] = checked
                        enabled: (delegateLoader.enabledKey ? session[delegateLoader.enabledKey] : true) && (root.checkedOptions > 1 || !checked)
                    }

                    Kirigami.Icon {
                        source: delegateLoader.icon
                        Layout.preferredWidth: Kirigami.Units.iconSizes.small
                        Layout.preferredHeight: Kirigami.Units.iconSizes.small
                    }

                    QQC2.Label {
                        text: delegateLoader.text
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        textFormat: Text.PlainText
                    }

                    QQC2.Label {
                        visible: !((delegateLoader.enabledKey ? session[delegateLoader.enabledKey] : true) && (root.checkedOptions > 1 || !checked))
                        text: i18n("Unavailable")
                        color: Kirigami.Theme.disabledTextColor
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignRight
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        Layout.rightMargin: Kirigami.Units.smallSpacing
                        elide: Text.ElideRight
                        textFormat: Text.PlainText
                    }
                }
            }
        }

        moveDisplaced: Transition {
            YAnimator {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutQuad
            }
        }
    }

    Component.onCompleted: {
        var actions = Data.data;

        var actionMap = {};
        actions.forEach(action => {
            actionMap[action.configKey] = action;
        });
        for (var i = 0; i < actions.length; i++) {
            actionMap[actions[i].configKey] = actions[i];
        }

        var order = cfg_actionsOrder && cfg_actionsOrder.length ? cfg_actionsOrder : [];
        if (!order.length) {
            actions.forEach(action => order.push(action.configKey));
            cfg_actionsOrder = order;
        }

        order.forEach(keyName => {
            var item = actionMap[keyName];
            if (!item) {
                return;
            }

            let key = "cfg_show_" + item.configKey;
            actionsModel.append({
                text: item.tooltip_mainText,
                icon: item.icon,
                cfgKey: key, // used for binding
                configKey: item.configKey, // used for reordering
                enabledKey: item.requires ? ("can" + item.requires) : ""
            });
            cfgKeys.push(key);
        });
    }
}
