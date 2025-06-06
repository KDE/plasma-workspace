/*
    SPDX-FileCopyrightText: 2018 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2022 Dominic Hayes <ferenosdev@outlook.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

import QtQuick
import QtQuick.Controls as QtControls
import org.kde.kirigami as Kirigami
import org.kde.newstuff as NewStuff
import org.kde.kcmutils as KCM
import org.kde.private.kcms.lookandfeel as Private

KCM.AbstractKCM {
    id: root
    framedView: false

    actions: [
        Kirigami.Action {
            id: automaticModeAction
            text: i18nc("@option:check", "Switch to Dark Mode at Night")
            checkable: true
            checked: kcm.settings.automaticLookAndFeel
            onTriggered: {
                kcm.settings.automaticLookAndFeel = checked;
                if (!kcm.settings.automaticLookAndFeel) {
                    kcm.save();
                }
            }
            displayComponent: QtControls.Switch {
                text: automaticModeAction.text
                checked: automaticModeAction.checked
                visible: automaticModeAction.visible
                onToggled: automaticModeAction.trigger()
            }
        },
        NewStuff.Action {
            configFile: "lookandfeel.knsrc"
            text: i18nc("@action:intoolbar", "Get New…")
            onEntryEvent: function (entry, event) {
                if (event == NewStuff.Entry.StatusChangedEvent) {
                    kcm.knsEntryChanged(entry);
                } else if (event == NewStuff.Entry.AdoptedEvent) {
                    kcm.reloadConfig();
                }
            }
        }
    ]

    QtControls.StackView {
        id: stackView
        anchors.fill: parent

        function reload() {
            if (kcm.settings.automaticLookAndFeel) {
                replace(null, "DayNightLookAndFeel.qml");
            } else {
                replace(null, "StaticLookAndFeel.qml");
            }
        }

        Connections {
            target: kcm.settings

            function onAutomaticLookAndFeelChanged() {
                stackView.reload();
            }
        }

        Component.onCompleted: reload();
    }
}
