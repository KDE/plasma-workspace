/*
    SPDX-FileCopyrightText: 2018 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2022 Dominic Hayes <ferenosdev@outlook.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

import QtQuick 2.6
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import QtQuick.Controls as QtControls
import org.kde.kirigami as Kirigami
import org.kde.newstuff 1.91 as NewStuff
import org.kde.kcmutils as KCM
import org.kde.private.kcms.lookandfeel 1.0 as Private

KCM.AbstractKCM {
    id: root

    actions: [
        Kirigami.Action {
            icon.name: "configure"
            text: i18nc("@action:intoolbar", "Choose what to apply…")
            onTriggered: kcm.push("ChooseWhatToApply.qml")
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

    ColumnLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.smallSpacing

        Kirigami.FormLayout {
            width: parent.width

            QtControls.CheckBox {
                text: i18nc("@option:check", "Switch between light and dark global themes depending on time of day")
                checked: kcm.settings.automaticLookAndFeel
                onClicked: kcm.settings.automaticLookAndFeel = checked;

                KCM.SettingStateBinding {
                    configObject: kcm.settings
                    settingName: "automaticLookAndFeel"
                }
            }
        }

        QtControls.StackView {
            id: stackView
            Layout.fillWidth: true
            Layout.fillHeight: true

            property bool complete: false

            function reload() {
                if (kcm.settings.automaticLookAndFeel) {
                    replace(null, "DayNightOptions.qml");
                } else {
                    replace(null, "SingleOptions.qml");
                }
            }

            Connections {
                target: kcm.settings

                function onAutomaticLookAndFeelChanged() {
                    if (stackView.complete) {
                        stackView.reload();
                    }
                }
            }

            Component.onCompleted: {
                reload();
                complete = true;
            }
        }
    }
}
