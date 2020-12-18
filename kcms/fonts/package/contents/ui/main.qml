/*
   Copyright (c) 2015 Antonis Tsiapaliokas <antonis.tsiapaliokas@kde.org>
   Copyright (c) 2017 Marco Martin <mart@kde.org>
   Copyright (c) 2019 Benjamin Port <benjamin.port@enioka.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.7 as QtControls
import QtQuick.Dialogs 1.2 as QtDialogs
import QtGraphicalEffects 1.12

import org.kde.kquickcontrolsaddons 2.0 // For KCMShell
import org.kde.kirigami 2.12 as Kirigami
import org.kde.kcm 1.3 as KCM

KCM.SimpleKCM {
    id: root

    KCM.ConfigModule.quickHelp: i18n("This module lets you configure the system fonts.")

    Kirigami.Action {
        id: kscreenAction
        visible: KCMShell.authorize("kcm_kscreen.desktop").length > 0
        text: i18n("Change Display Scaling...")
        iconName: "preferences-desktop-display"
        onTriggered: KCMShell.open("kcm_kscreen.desktop")
    }

    // use fontmetrics directly to make the spacing based off of the gridunit
    // as it would be if calculated by currently selected font in KCM
    FontMetrics {
        id: fontMetrics
        font: kcm.fontsSettings.font

        property real gridUnit: fontMetrics.height
        property int smallSpacing: Math.floor(gridUnit / 4)
        property int largeSpacing: smallSpacing * 2
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Item {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true

            Layout.maximumWidth: 45 * fontMetrics.height
            Layout.preferredHeight: 15 * fontMetrics.height

            Image {
                id: backgroundImage
                anchors.fill: parent

                source: kcm.wallpaperLocation
                fillMode: Image.PreserveAspectCrop
                mipmap: true

                PreviewWindow {}
            }

            InnerShadow {
                anchors.fill: backgroundImage
                cached: true
                horizontalOffset: 1
                verticalOffset: 1
                radius: 24
                samples: 24
                color: "black"
                smooth: true
                source: backgroundImage
            }

        }

        Kirigami.InlineMessage {
            id: hugeFontsMessage
            Layout.fillWidth: true
            showCloseButton: true
            text: i18n("Very large fonts may produce odd-looking results. Consider adjusting the global screen scale instead of using a very large font size.")

            Connections {
                target: kcm
                function onFontsHaveChanged() {
                    hugeFontsMessage.visible = generalFontWidget.font.pointSize > 14
                    || fixedWidthFontWidget.font.pointSize > 14
                    || smallFontWidget.font.pointSize > 14
                    || toolbarFontWidget.font.pointSize > 14
                    || menuFontWidget.font.pointSize > 14
                }
            }

            actions: [ kscreenAction ]
        }

        Kirigami.InlineMessage {
            id: dpiTwiddledMessage
            Layout.fillWidth: true
            showCloseButton: true
            text: i18n("The recommended way to scale the user interface is using the global screen scaling feature.")
            actions: [ kscreenAction ]
        }

        Kirigami.FormLayout {
            FontWidget {
                id: generalFontWidget
                Kirigami.FormData.label: i18n("General:")
                tooltipText: i18n("Select general font")
                category: "font"
                font: kcm.fontsSettings.font

                KCM.SettingStateBinding {
                    configObject: kcm.fontsSettings
                    settingName: "font"
                }
            }
            FontWidget {
                id: fixedWidthFontWidget
                Kirigami.FormData.label: i18n("Fixed width:")
                tooltipText: i18n("Select fixed width font")
                category: "fixed"
                font: kcm.fontsSettings.fixed

                KCM.SettingStateBinding {
                    configObject: kcm.fontsSettings
                    settingName: "fixed"
                }
            }
            FontWidget {
                id: smallFontWidget
                Kirigami.FormData.label: i18n("Small:")
                tooltipText: i18n("Select small font")
                category: "smallestReadableFont"
                font: kcm.fontsSettings.smallestReadableFont

                KCM.SettingStateBinding {
                    configObject: kcm.fontsSettings
                    settingName: "smallestReadableFont"
                }
            }
            FontWidget {
                id: toolbarFontWidget
                Kirigami.FormData.label: i18n("Toolbar:")
                tooltipText: i18n("Select toolbar font")
                category: "toolBarFont"
                font: kcm.fontsSettings.toolBarFont

                KCM.SettingStateBinding {
                    configObject: kcm.fontsSettings
                    settingName: "toolBarFont"
                }
            }
            FontWidget {
                id: menuFontWidget
                Kirigami.FormData.label: i18n("Menu:")
                tooltipText: i18n("Select menu font")
                category: "menuFont"
                font: kcm.fontsSettings.menuFont

                KCM.SettingStateBinding {
                    configObject: kcm.fontsSettings
                    settingName: "menuFont"
                }
            }
            FontWidget {
                Kirigami.FormData.label: i18n("Window title:")
                tooltipText: i18n("Select window title font")
                category: "activeFont"
                font: kcm.fontsSettings.activeFont

                KCM.SettingStateBinding {
                    configObject: kcm.fontsSettings
                    settingName: "activeFont"
                }
            }
        }

        QtDialogs.FontDialog {
            id: fontDialog
            title: i18n("Select Font")
            modality: Qt.WindowModal
            property string currentCategory
            property bool adjustAllFonts: false
            onAccepted: {
                if (adjustAllFonts) {
                    kcm.adjustAllFonts()
                } else {
                    kcm.adjustFont(font, currentCategory)
                }
            }
        }
    }

    footer: RowLayout {
        QtControls.Button {
            id: adjustAllFontsButton
            icon.name: "font-select-symbolic"
            text: i18n("&Adjust All Fonts...")

            onClicked: kcm.adjustAllFonts();
            enabled: !kcm.fontsSettings.isImmutable("font")
                    || !kcm.fontsSettings.isImmutable("fixed")
                    || !kcm.fontsSettings.isImmutable("smallestReadableFont")
                    || !kcm.fontsSettings.isImmutable("toolBarFont")
                    || !kcm.fontsSettings.isImmutable("menuFont")
                    || !kcm.fontsSettings.isImmutable("activeFont")

            Layout.alignment: Qt.AlignHCenter
            Layout.columnSpan: 2
        }

        Item {
            Layout.fillWidth: true
        }

        QtControls.Button {
            icon.name: "configure"
            text: i18n("Configure Anti-Aliasing...")
            onClicked: kcm.push("AntiAliasingPage.qml");
        }
    }
}
