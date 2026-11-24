/*
    SPDX-FileCopyrightText: 2015 Antonis Tsiapaliokas <antonis.tsiapaliokas@kde.org>
    SPDX-FileCopyrightText: 2017 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2019 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QtControls
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM
import org.kde.config // KAuthorized

import org.kde.plasma.kcm.fonts as FontsKCM

KCM.SimpleKCM {
    id: root

    readonly property bool usingHugeFont: generalFontWidget.font.pointSize > 14
        || fixedWidthFontWidget.font.pointSize > 14
        || smallFontWidget.font.pointSize > 14
        || toolbarFontWidget.font.pointSize > 14
        || menuFontWidget.font.pointSize > 14

    readonly property bool usingInadvisablySmallFont: generalFontWidget.font.pointSize < 7
        || fixedWidthFontWidget.font.pointSize < 7
        // Deliberately not checking for Small font here since it's designed to be smaller
        || toolbarFontWidget.font.pointSize < 7
        || menuFontWidget.font.pointSize < 7

    readonly property bool usingDisplayFont: [
        generalFontWidget.font.family,
        fixedWidthFontWidget.font.family,
        smallFontWidget.font.family,
        toolbarFontWidget.font.family,
        menuFontWidget.font.family
    ].some(a => a.includes("Display") || a.includes(i18nc("Sub-string in a font name; 'Display' as in display font — a type of font inappropriate for computer screens", "Display")))

    Kirigami.Action {
        id: kscreenAction
        visible: KAuthorized.authorizeControlModule("kcm_kscreen")
        text: i18nc("@action:button", "Adjust Global Scale…")
        icon.name: "preferences-desktop-display"
        onTriggered: KCM.KCMLauncher.open("kcm_kscreen")
    }

    headerPaddingEnabled: false // Let the InlineMessages touch the edges
    header: ColumnLayout {
        spacing: 0

        Kirigami.InlineMessage {
            id: hugeFontsMessage
            Layout.fillWidth: true
            position: Kirigami.InlineMessage.Position.Header
            type: Kirigami.MessageType.Warning
            showCloseButton: true
            text: i18nc("@info:usagetip InlineMessage", "Very large fonts may produce odd-looking results. Instead of using a very large font size, consider adjusting the global screen scale.")

            Connections {
                target: kcm
                function onFontsHaveChanged() {
                    hugeFontsMessage.visible = root.usingHugeFont;
                }
            }

            actions: [ kscreenAction ]
        }

        Kirigami.InlineMessage {
            id: displayFontsMessage
            Layout.fillWidth: true
            position: Kirigami.InlineMessage.Position.Header
            type: Kirigami.MessageType.Warning
            showCloseButton: true
            text: xi18nc("@info", "“Display” fonts are <link url='https://wikipedia.org/wiki/Display_typeface'>not intended for use on computer displays</link>, and may produce odd results. Consider using a different font.")

            onLinkActivated: (url) => Qt.openUrlExternally(url)

            Connections {
                target: kcm
                function onFontsHaveChanged() {
                    displayFontsMessage.visible = root.usingDisplayFont;
                }
            }
        }

        Kirigami.InlineMessage {
            id: fontTooSmallMessage
            Layout.fillWidth: true
            position: Kirigami.InlineMessage.Position.Header
            type: Kirigami.MessageType.Warning
            showCloseButton: true
            text: i18nc("@info:usagetip", "Plasma is not designed to be usable with fonts smaller than 4pt. Size has been reset to 4pt.")

            Connections {
                target: kcm
                function onFontTooSmall() {
                    fontTooSmallMessage.visible = true
                }
            }
        }

        Kirigami.InlineMessage {
            id: fontInadvisablySmall
            Layout.fillWidth: true
            position: Kirigami.InlineMessage.Position.Header
            type: Kirigami.MessageType.Warning
            showCloseButton: true
            text: i18nc("@info:usagetip", "Very small fonts may produce odd-looking results. Instead of using a very small font size, consider adjusting the global screen scale.")

            actions: [ kscreenAction ]

            Connections {
                target: kcm
                function onFontsHaveChanged() {
                    fontInadvisablySmall.visible = !fontTooSmallMessage.visible && root.usingInadvisablySmallFont;
                }
            }
        }
    }

    FontsKCM.DevicePixelRatioHelper {
        id: dprHelper
        window: root.Window.window
    }

    Kirigami.Form {
        Kirigami.FormGroup {
            FontWidget {
                id: generalFontWidget
                Layout.fillWidth: true
                label: i18nc("@title:group General system font", "General:")
                tooltipText: i18nc("@action:button opens dialog, tooltip/accessible only", "Select general font")
                category: "font"
                font: kcm.fontsSettings.font

                KCM.SettingStateBinding {
                    configObject: kcm.fontsSettings
                    settingName: "font"
                }
            }
            FontWidget {
                id: fixedWidthFontWidget
                Layout.fillWidth: true
                label: i18nc("@title:group Fixed width font", "Fixed width:")
                tooltipText: i18nc("@action:button opens dialog, tooltip/accessible only", "Select fixed width font")
                category: "fixed"
                font: kcm.fontsSettings.fixed

                KCM.SettingStateBinding {
                    configObject: kcm.fontsSettings
                    settingName: "fixed"
                }
            }
            FontWidget {
                id: smallFontWidget
                Layout.fillWidth: true
                label: i18nc("@title:group Small font", "Small:")
                tooltipText: i18nc("@action:button opens dialog, tooltip/accessible only", "Select small font")
                category: "smallestReadableFont"
                font: kcm.fontsSettings.smallestReadableFont

                KCM.SettingStateBinding {
                    configObject: kcm.fontsSettings
                    settingName: "smallestReadableFont"
                }
            }
            FontWidget {
                id: toolbarFontWidget
                Layout.fillWidth: true
                label: i18nc("@title:group Toolbar font", "Toolbar:")
                tooltipText: i18nc("@action:button opens dialog, tooltip/accessible only", "Select toolbar font")
                category: "toolBarFont"
                font: kcm.fontsSettings.toolBarFont

                KCM.SettingStateBinding {
                    configObject: kcm.fontsSettings
                    settingName: "toolBarFont"
                }
            }
            FontWidget {
                id: menuFontWidget
                Layout.fillWidth: true
                label: i18nc("@title:group Menu font", "Menu:")
                tooltipText: i18nc("@action:button opens dialog, tooltip/accessible only", "Select menu font")
                category: "menuFont"
                font: kcm.fontsSettings.menuFont

                KCM.SettingStateBinding {
                    configObject: kcm.fontsSettings
                    settingName: "menuFont"
                }
            }
            FontWidget {
                Layout.fillWidth: true
                label: i18nc("@title:group Window title font", "Window title:")
                tooltipText: i18nc("@action:button opens dialog, tooltip/accessible only", "Select window title font")
                category: "activeFont"
                font: kcm.fontsSettings.activeFont

                KCM.SettingStateBinding {
                    configObject: kcm.fontsSettings
                    settingName: "activeFont"
                }
            }
            Kirigami.FormEntry {
                contentItem: Item { /*dummy, we just want a trailingItem*/ }
                trailingItems: [
                    QtControls.Button {
                        id: adjustAllFontsButton
                        Layout.alignment: Qt.AlignRight
                        icon.name: "font-select-symbolic"
                        text: i18nc("@action:button opens dialog", "&Adjust All Fonts…")

                        onClicked: kcm.adjustAllFonts();
                        enabled: !kcm.fontsSettings.isImmutable("font")
                            || !kcm.fontsSettings.isImmutable("fixed")
                            || !kcm.fontsSettings.isImmutable("smallestReadableFont")
                            || !kcm.fontsSettings.isImmutable("toolBarFont")
                            || !kcm.fontsSettings.isImmutable("menuFont")
                            || !kcm.fontsSettings.isImmutable("activeFont")
                    }
                ]
            }
        }

        Kirigami.FormGroup{
            Kirigami.FormEntry {
                title: i18nc("@title:group", "Anti-Aliasing:")
                contentItem: QtControls.CheckBox {
                    id: antiAliasingCheckBox
                    checked: kcm.fontsAASettings.antiAliasing
                    onToggled: kcm.fontsAASettings.antiAliasing = checked
                    text: i18nc("@option:check completes Anti-Aliasing: …", "Enable")

                    KCM.SettingStateBinding {
                        configObject: kcm.fontsAASettings
                        settingName: "antiAliasing"
                        extraEnabledConditions: !kcm.fontsAASettings.isAaImmutable
                    }
                }
                trailingItems: [
                    Kirigami.ContextualHelpButton {
                        toolTipText: xi18nc("@info:tooltip Anti-Aliasing", "Pixels on displays are generally aligned in a grid. Therefore shapes of fonts that do not align with this grid will look blocky and wrong unless <emphasis>anti-aliasing</emphasis> techniques are used to reduce this effect. You generally want to keep this option enabled unless it causes problems.")
                    }
                ]
            }
            Kirigami.FormEntry {
                enabled: antiAliasingCheckBox.checked
                contentItem: QtControls.CheckBox {
                    id: excludeCheckBox
                    leftPadding: mirrored ? padding : antiAliasingCheckBox.indicator.width + antiAliasingCheckBox.spacing
                    rightPadding: !mirrored ? padding : antiAliasingCheckBox.indicator.width + antiAliasingCheckBox.spacing
                    checked: kcm.fontsAASettings.exclude
                    onToggled: kcm.fontsAASettings.exclude = checked;
                    text: i18nc("@option:check followed by spinboxes; Exclude from x pt to y pt (from anti-aliasing)", "Exclude range from")

                    KCM.SettingStateBinding {
                        configObject: kcm.fontsAASettings
                        settingName: "exclude"
                        extraEnabledConditions: !kcm.fontsAASettings.isAaImmutable && antiAliasingCheckBox.checked
                    }
                }
                trailingItems: [
                    QtControls.SpinBox {
                        id: excludeFromSpinBox
                        Layout.fillWidth: true
                        stepSize: 1
                        onValueModified: kcm.fontsAASettings.excludeFrom = value
                        textFromValue: function(value, locale) { return i18nc("@label:valuesuffix, font point size", "%1 pt", value)}
                        valueFromText: function(text, locale) { return parseInt(text) }
                        editable: true
                        value: kcm.fontsAASettings.excludeFrom

                        KCM.SettingStateBinding {
                            configObject: kcm.fontsAASettings
                            settingName: "excludeFrom"
                            extraEnabledConditions: excludeCheckBox.checked
                        }
                    },

                    QtControls.Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: i18nc("@label:spinbox, part of Exclude from … to …", "to")
                        textFormat: Text.PlainText
                        enabled: excludeCheckBox.checked
                    },

                    QtControls.SpinBox {
                        id: excludeToSpinBox
                        Layout.fillWidth: true
                        stepSize: 1
                        onValueModified: kcm.fontsAASettings.excludeTo = value
                        textFromValue: function(value, locale) { return i18nc("@label:valuesuffix, font point size", "%1 pt", value)}
                        valueFromText: function(text, locale) { return parseInt(text) }
                        editable: true
                        value: kcm.fontsAASettings.excludeTo

                        KCM.SettingStateBinding {
                            configObject: kcm.fontsAASettings
                            settingName: "excludeTo"
                            extraEnabledConditions: excludeCheckBox.checked
                        }
                    }
                ]
            }

            Kirigami.FormSeparator { }

            Kirigami.FormEntry {
                title: i18nc("Used as a noun, and precedes a combobox full of options", "Sub-pixel rendering:")
                contentItem: QtControls.ComboBox {
                    id: subPixelCombo
                    Layout.preferredWidth: Math.max(subPixelCombo.implicitWidth, hintingCombo.implicitWidth)
                    currentIndex: kcm.subPixelCurrentIndex
                    onActivated: (index) => {
                        kcm.subPixelCurrentIndex = index
                    }
                    model: kcm.subPixelOptionsModel
                    textRole: "display"
                    popup.width: popup.implicitWidth
                    delegate: QtControls.ItemDelegate {
                        id: subPixelDelegate
                        contentItem: ColumnLayout {
                            id: subPixelLayout
                            Kirigami.Heading {
                                id: subPixelComboText
                                text: model.display
                                textFormat: Text.PlainText
                                level: 5
                            }
                            Image {
                                id: subPixelComboImage
                                Layout.preferredWidth: implicitWidth / dprHelper.devicePixelRatio
                                Layout.preferredHeight: implicitHeight / dprHelper.devicePixelRatio
                                source: kcm.imageProviderReady ? "image://preview/" + model.index + "_" + kcm.hintingCurrentIndex + "_" + dprHelper.devicePixelRatio : ""
                                asynchronous: true
                            }
                        }
                    }

                    KCM.SettingStateBinding {
                        configObject: kcm.fontsAASettings
                        settingName: "subPixel"
                        extraEnabledConditions: antiAliasingCheckBox.checked && !kcm.fontsAASettings.isAaImmutable
                    }
                }
                trailingItems: [
                    Kirigami.ContextualHelpButton {
                        toolTipText: xi18nc("@info:tooltip Sub-pixel rendering", "<para>On TFT or LCD screens every single pixel is actually composed of three or four smaller monochrome lights. These <emphasis>sub-pixels</emphasis> can be changed independently to further improve the quality of displayed fonts.</para> <para>The rendering quality is only improved if the selection matches the manner in which the sub-pixels of your display are aligned. Most displays have a linear ordering of <emphasis>RGB</emphasis> sub-pixels, some have <emphasis>BGR</emphasis> and some exotic orderings are not supported by this feature.</para>This does not work with CRT monitors.")
                    }
                ]
            }
            Kirigami.FormEntry {
                title: i18nc("Used as a noun, and precedes a combobox full of options", "Hinting:")
                contentItem: QtControls.ComboBox {
                    id: hintingCombo
                    Layout.preferredWidth: Math.max(subPixelCombo.implicitWidth, hintingCombo.implicitWidth)
                    currentIndex: kcm.hintingCurrentIndex
                    onActivated: (index) => {
                        kcm.hintingCurrentIndex = index
                    }
                    model: kcm.hintingOptionsModel
                    textRole: "display"
                    popup.width: popup.implicitWidth
                    delegate: QtControls.ItemDelegate {
                        id: hintingDelegate
                        contentItem: ColumnLayout {
                            id: hintingLayout
                            Kirigami.Heading {
                                id: hintingComboText
                                text: model.display
                                textFormat: Text.PlainText
                                level: 5
                            }
                            Image {
                                id: hintingComboImage
                                Layout.preferredWidth: implicitWidth / dprHelper.devicePixelRatio
                                Layout.preferredHeight: implicitHeight / dprHelper.devicePixelRatio
                                source: kcm.imageProviderReady ? "image://preview/" + kcm.subPixelCurrentIndex + "_" + model.index + "_" + dprHelper.devicePixelRatio : ""
                                asynchronous: true
                            }
                        }
                    }
                    KCM.SettingStateBinding {
                        configObject: kcm.fontsAASettings
                        settingName: "hinting"
                        extraEnabledConditions: antiAliasingCheckBox.checked && !kcm.fontsAASettings.isAaImmutable
                    }
                }
                trailingItems: [
                    Kirigami.ContextualHelpButton {
                        toolTipText: xi18nc("@info:tooltip Hinting", "Hinting is a technique in which hints embedded in a font are used to enhance the rendering quality especially at small sizes. Stronger hinting generally leads to sharper edges but the small letters will less closely resemble their shape at big sizes.")
                    }
                ]
            }
        }
    }
}
