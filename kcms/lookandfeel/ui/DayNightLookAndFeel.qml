/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QtControls
import org.kde.config as KConfig
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM
import org.kde.private.kcms.lookandfeel as Private

Rectangle {
    KCM.ConfigModule.buttons: KCM.ConfigModule.Default | KCM.ConfigModule.Help | KCM.ConfigModule.Apply
    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false
    color: Kirigami.Theme.backgroundColor

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
        anchors.margins: 6
        spacing: Kirigami.Units.smallSpacing

        QtControls.Label {
            Layout.fillWidth: true
            text: i18nc("@info", "The chosen light Global Theme will be used during the day, and the dark one will be used at night.")
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        QtControls.Button {
            Layout.alignment: Qt.AlignHCenter
            enabled: KConfig.KAuthorized.authorizeControlModule("kcm_nighttime")
            text: i18nc("@action:button Configure day-night cycle times", "Configure Day/Night Cycle…")
            icon.name: "configure"
            onClicked: KCM.KCMLauncher.open("kcm_nighttime")
        }

        Row {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

            LookAndFeelBox {
                id: lightLookAndFeelRadioButton
                preview: lightLookAndFeelInformation.preview
                text: i18nc("@option:radio Light global theme", "Light")

                onClicked: {
                    kcm.push("LookAndFeelSelector.qml", {
                        "variant": Private.LookAndFeel.Variant.Light,
                    });
                }
            }

            LookAndFeelBox {
                id: darkLookAndFeelRadioButton
                preview: darkLookAndFeelInformation.preview
                text: i18nc("@option:radio Dark global theme", "Dark")

                onClicked: {
                    kcm.push("LookAndFeelSelector.qml", {
                        "variant": Private.LookAndFeel.Variant.Dark,
                    });
                }
            }
        }

        Kirigami.FormLayout {
            width: parent.width

            RowLayout {
                spacing: Kirigami.Units.smallSpacing

                QtControls.CheckBox {
                    text: i18nc("@option:check", "Wait until idleness before switching:")
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
                        extraEnabledConditions: kcm.settings.automaticLookAndFeelOnIdle
                    }
                }

                Kirigami.ContextualHelpButton {
                    toolTipText: i18nc("@info:tooltip", "Minimizes interruptions when actively using the computer.")
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
