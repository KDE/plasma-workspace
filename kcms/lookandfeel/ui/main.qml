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

    Private.LookAndFeelInformation {
        id: selectedLookAndFeelInformation
        model: kcm.model
        packageId: kcm.settings.lookAndFeelPackage
    }

    Private.LookAndFeelInformation {
        id: lightLookAndFeelInformation
        model: kcm.model
        packageId: kcm.settings.defaultLightLookAndFeel
    }

    Private.LookAndFeelInformation {
        id: darkLookAndFeelInformation
        model: kcm.model
        packageId: kcm.settings.defaultDarkLookAndFeel
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.smallSpacing

        Row {
            Layout.alignment: Qt.AlignHCenter
            spacing: Kirigami.Units.largeSpacing

            QtControls.ButtonGroup { id: themeGroup }

            LookAndFeelBox {
                id: lightLookAndFeelRadioButton
                preview: lightLookAndFeelInformation.preview
                text: i18nc("@option:radio Light global theme", "Light")
                checked: selectedLookAndFeelInformation.variant !== "dark"
                group: themeGroup

                onToggled: {
                    kcm.settings.lookAndFeelPackage = kcm.settings.defaultLightLookAndFeel;
                }

                onExpanded: {
                    kcm.push("LookAndFeelSelector.qml", {
                        "variant": "light",
                    });
                }
            }

            LookAndFeelBox {
                id: darkLookAndFeelRadioButton
                preview: darkLookAndFeelInformation.preview
                text: i18nc("@option:radio Dark global theme", "Dark")
                checked: selectedLookAndFeelInformation.variant === "dark"
                group: themeGroup

                onToggled: {
                    kcm.settings.lookAndFeelPackage = kcm.settings.defaultDarkLookAndFeel;
                }

                onExpanded: {
                    kcm.push("LookAndFeelSelector.qml", {
                        "variant": "dark",
                    });
                }
            }
        }

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

            RowLayout {
                enabled: kcm.settings.automaticLookAndFeel
                spacing: Kirigami.Units.smallSpacing

                Item {
                    width: Kirigami.Units.gridUnit
                }

                QtControls.CheckBox {
                    text: i18nc("@option:check", "Minimize interruptions by switching between themes when computer is idle:")
                    checked: kcm.settings.automaticLookAndFeelOnIdle
                    onClicked: kcm.settings.automaticLookAndFeelOnIdle = checked;

                    KCM.SettingStateBinding {
                        configObject: kcm.settings
                        settingName: "automaticLookAndFeelOnIdle"
                    }
                }

                QtControls.SpinBox {
                    from: 1
                    value: kcm.settings.automaticLookAndFeelIdleInterval
                    textFromValue: (value, locale) => i18ncp("@item:valuesuffix idle interval", "%1 second", "%1 seconds", value)
                    valueFromText: (text, locale) => parseInt(text)
                    onValueModified: kcm.settings.automaticLookAndFeelIdleInterval = value;

                    KCM.SettingStateBinding {
                        configObject: kcm.settings
                        settingName: "automaticLookAndFeelIdleInterval"
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
