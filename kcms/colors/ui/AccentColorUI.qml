/*
    SPDX-FileCopyrightText: 2023 Tanbir Jishan <tantalising007@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.1
import QtQuick.Dialogs as QtDialogs
import QtQuick.Controls 2.3 as QQC2
import QtQuick.Templates 2.3 as T

import org.kde.kirigami as Kirigami

RowLayout {
    id: accentColorUI

    readonly property int colorButtonSize: Math.round(Kirigami.Units.gridUnit * 1.25)

    spacing: Kirigami.Units.smallSpacing

    QQC2.ComboBox {
        id: colorModeBox
        Layout.alignment: Qt.AlignTop | Qt.AlignRight

        model: [
            i18nc("@item:inlistbox Accent color from color scheme", "Accent color from color scheme"),
            i18nc("@item:inlistbox Accent color from wallpaper", "Accent color from wallpaper"),
            i18nc("@item:inlistbox User-chosen custom accent color", "Custom accent color")
        ]

        currentIndex: {
            if (kcm.accentColorFromWallpaper) return 1;
            return Qt.colorEqual(kcm.accentColor, "transparent") ? 0 : 2;
        }

        onCurrentIndexChanged: {
            switch (currentIndex) {
                case 0: // colorScheme
                    kcm.accentColor = "transparent";
                    kcm.accentColorFromWallpaper = false;
                    break;

                case 1: // wallpaper
                    kcm.accentColorFromWallpaper = true;
                    break;

                case 2: // custom
                    if(!colorRepeater.model.some(color => Qt.colorEqual(color.color, kcm.accentColor))) { // if not accent is from the provided list, assign the last used color, if any, or a sensible value
                        kcm.accentColor = Qt.colorEqual(kcm.lastUsedCustomAccentColor, "transparent") ? colorRepeater.model[0].color : kcm.lastUsedCustomAccentColor;
                        kcm.accentColorFromWallpaper = false;
                    }
                    break;
            }
        }
    }

    Flow {
        Layout.fillWidth: true
        Layout.maximumWidth: (colorRepeater.count * (accentColorUI.colorButtonSize + spacing)) + customColorIndicator.width

        spacing: Kirigami.Units.largeSpacing
        opacity: colorModeBox.currentIndex === 2 ? 1.0 : 0.5

        component ColorRadioButton : T.RadioButton {
            id: control
            autoExclusive: false

            property color color: "transparent"
            property bool highlight: true

            implicitWidth: accentColorUI.colorButtonSize
            implicitHeight: accentColorUI.colorButtonSize

            background: Rectangle {
                readonly property bool showHighlight: parent.hovered && !control.checked && control.highlight
                radius:  showHighlight ? Kirigami.Units.cornerRadius : Math.round(height / 2)
                color: control.color
                border {
                    color: showHighlight ? Kirigami.Theme.highlightColor : Qt.rgba(0, 0, 0, 0.15)
                }
                Behavior on radius {
                    PropertyAnimation {
                        duration: Kirigami.Units.veryShortDuration
                        from: Math.round(height / 2)
                    }
                }
                Rectangle {
                    id: tabHighlight
                    anchors.fill: parent
                    radius: Math.round(height / 2)
                    scale: 1.3
                    color: "transparent"
                    visible: control.visualFocus
                    border {
                        color: Kirigami.Theme.highlightColor
                        width: 1
                    }
                }
            }

            indicator: Rectangle {
                radius: height / 2
                visible: control.checked
                anchors {
                    fill: parent
                    margins: Math.round(Kirigami.Units.smallSpacing * 1.25)
                }
                border {
                    color: Qt.rgba(0, 0, 0, 0.15)
                    width: 1
                }
            }
        }

        Repeater {
            id: colorRepeater

            model: [
                {"name": i18nc("color name, pick a fun name for #e93a9a in your language (does not have to be a literal translation)", "Feisty Flamingo"),   "color": "#e93a9a"},
                {"name": i18nc("color name, pick a fun name for #e93d58 in your language (does not have to be a literal translation)", "Dragon's Fruit"),    "color": "#e93d58"},
                {"name": i18nc("color name, pick a fun name for #e9643a in your language (does not have to be a literal translation)", "Sweet Potato"),      "color": "#e9643a"},
                // {"name": i18nc("color name, pick a fun name for #ef973c in your language (does not have to be a literal translation)", "Ambient Amber"),  "color": "#ef973c"},
                {"name": i18nc("color name, pick a fun name for #e8cb2d in your language (does not have to be a literal translation)", "Sparkle Sunbeam"),   "color": "#e8cb2d"},
                // {"name": i18nc("color name, pick a fun name for #b6e521 in your language (does not have to be a literal translation)", "Lemon-Lime"),     "color": "#b6e521"},
                {"name": i18nc("color name, pick a fun name for #3dd425 in your language (does not have to be a literal translation)", "Verdant Charm"),     "color": "#3dd425"},
                // {"name": i18nc("color name, pick a fun name for #00d485 in your language (does not have to be a literal translation)", "Mellow Meadow"),  "color": "#00d485"},
                {"name": i18nc("color name, pick a fun name for #00d3b8 in your language (does not have to be a literal translation)", "Tepid Teal"),        "color": "#00d3b8"},
                {"name": i18nc("color name, pick a fun name for #3daee9 in your language (does not have to be a literal translation)", "Plasma Blue"),       "color": "#3daee9"},
                {"name": i18nc("color name, pick a fun name for #b875dc in your language (does not have to be a literal translation)", "Pon Purple"),        "color": "#b875dc"},
                {"name": i18nc("color name, pick a fun name for #926ee4 in your language (does not have to be a literal translation)", "Bajo Purple"),       "color": "#926ee4"},
                {"name": i18nc("color name, pick a fun name for #686b6f in your language (does not have to be a literal translation)", "Summer Shade"),      "color": "#686b6f"},
                // {"name": i18nc("color name, pick a fun name for #232629 in your language (does not have to be a literal translation)", "Burnt Charcoal"), "color": "#232629"},
                // {"name": i18nc("color name, pick a fun name for #cb775a in your language (does not have to be a literal translation)", "Cafétera Brown"), "color": "#cb775a"},
                // {"name": i18nc("color name, pick a fun name for #6a250e in your language (does not have to be a literal translation)", "Rich Hardwood"),  "color": "#6a250e"}
            ]

            delegate: ColorRadioButton {
                id: control
                color: modelData.color
                checked: Qt.colorEqual(kcm.accentColor, modelData.color) && !kcm.accentColorFromWallpaper

                QQC2.ToolTip {
                    text: modelData.name
                    visible: control.hovered || control.visualFocus
                    delay: -1
                }

                onToggled: {
                    kcm.accentColorFromWallpaper = false;
                    kcm.accentColor = modelData.color;
                    kcm.lastUsedCustomAccentColor = modelData.color;
                    checked = Qt.binding(() => Qt.colorEqual(kcm.accentColor, modelData.color) && !kcm.accentColorFromWallpaper);
                }
            }
        }

        QtDialogs.ColorDialog {
            id: colorDialog
            title: i18n("Choose custom accent color")
            // User must either choose a colour or cancel the operation before doing something else
            modality: Qt.ApplicationModal
            parentWindow: accentColorUI.Window.window
            selectedColor: Qt.colorEqual(kcm.lastUsedCustomAccentColor, "transparent") ? kcm.accentColor : kcm.lastUsedCustomAccentColor
            onAccepted: {
                kcm.accentColor = colorDialog.selectedColor;
                kcm.lastUsedCustomAccentColor = colorDialog.selectedColor;
                kcm.accentColorFromWallpaper = false;
            }
        }

        Rectangle {
            id: customColorIndicator

            height: customColorButton.height
            width: customColorButton.visibleWidth + colorPicker.width
            radius: customColorButton.background.radius
            color: complementary(kcm.accentColor)

            function complementary(color) {
                return Qt.hsla((((color.hslHue + 0.5) % 1) + 1) % 1, color.hslSaturation, color.hslLightness, color.a);
            }


            ColorRadioButton {
                id: customColorButton

                readonly property bool isCustomColor: !kcm.accentColorFromWallpaper
                    && !Qt.colorEqual(kcm.accentColor, "transparent")
                    && !colorRepeater.model.some(color => Qt.colorEqual(color.color, root.accentColor))

                readonly property real visibleWidth: visible ? width + Kirigami.Units.smallSpacing : 0

                color:  kcm.accentColor
                highlight: false
                checked: isCustomColor
                visible: isCustomColor
                anchors.left: parent.left

                MouseArea { // To prevent the button being toggled when clicked. The toggle state should only be controlled through isCustomColor
                    anchors.fill: parent
                    onClicked: colorDialog.open()
                }
                onFocusChanged: colorPicker.focus = true;
            }
            QQC2.RoundButton {
                id: colorPicker
                anchors.right: parent.right
                height: customColorButton.height
                width: customColorButton.width
                padding: 0  // Round button adds some padding by default which we don't need. Setting this to 0 centers the icon
                icon.name: "color-picker"
                icon.width : Math.round(Kirigami.Units.iconSizes.small * 0.9) // This provides a nice padding.
                onClicked: colorDialog.open()
            }
        }
    }
}
