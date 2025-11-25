/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2015 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2025 Shubham Arora <contact@shubhamarora.dev>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0
import QtQuick.Layouts
import QtQuick.Controls 2.5 as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.private.sessions 2.0
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
    property var cfgKeys: []

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

        headerPositioning: ListView.OverlayHeader
        header: Kirigami.InlineViewHeader {
            width: list.width
            text: i18nc("@title:column", "Actions")
        }

        delegate: Kirigami.SwipeListItem {
            id: delegateItem
            width: list.width
            
            Kirigami.Theme.useAlternateBackgroundColor: true

            highlighted: false
            hoverEnabled: false
            down: false

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Icon {
                    source: model.icon
                    Layout.preferredWidth: Kirigami.Units.iconSizes.small
                    Layout.preferredHeight: Kirigami.Units.iconSizes.small
                }

                QQC2.Label {
                    text: model.text
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                QQC2.CheckBox {
                    visible: (model.enabledKey ? session[model.enabledKey] : true)
                    checked: root[model.cfgKey]
                    onToggled: root[model.cfgKey] = checked
                    enabled: (model.enabledKey ? session[model.enabledKey] : true) && (root.checkedOptions > 1 || !checked)
                }

                QQC2.Label {
                    visible: !((model.enabledKey ? session[model.enabledKey] : true) && (root.checkedOptions > 1 || !checked))
                    text: i18n("Unavailable")
                    color: Kirigami.Theme.disabledTextColor
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    Layout.rightMargin: Kirigami.Units.smallSpacing
                    elide: Text.ElideRight
                }
            }
        }
    }

    Component.onCompleted: {
        var actions = Data.data;
        for (var i = 0; i < actions.length; i++) {
            var item = actions[i];
            var key = "cfg_show_" + item.configKey;
            actionsModel.append({
                text: item.tooltip_mainText,
                icon: item.icon,
                cfgKey: key,
                enabledKey: item.requires ? ("can" + item.requires) : ""
            });
            cfgKeys.push(key);
        }
    }
}
