/*
    SPDX-FileCopyrightText: 2015 Antonis Tsiapaliokas <antonis.tsiapaliokas@kde.org>
    SPDX-FileCopyrightText: 2017 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2019 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.0 as QtControls
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM
import org.kde.config // KAuthorized

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
    ].find(a => a.includes("Display") || a.includes(i18nc("Sub-string in a font name; 'Display' as in display font — a type of font inappropriate for computer screens", "Display")))

    Kirigami.Action {
        id: kscreenAction
        visible: KAuthorized.authorizeControlModule("kcm_kscreen")
        text: i18n("Adjust Global Scale…")
        icon.name: "preferences-desktop-display"
        onTriggered: KCM.KCMLauncher.open("kcm_kscreen")
    }

    headerPaddingEnabled: false // Let the InlineMessages touch the edges
    header: ColumnLayout {
        spacing: 0

        Kirigami.InlineMessage {
            id: antiAliasingMessage
            Layout.fillWidth: true
            position: Kirigami.InlineMessage.Position.Header
            showCloseButton: true
            text: i18n("Some changes such as anti-aliasing or DPI will only affect newly started applications.")

            Connections {
                target: kcm
                function onAliasingChangeApplied() {
                    antiAliasingMessage.visible = true
                }
            }
        }

        Kirigami.InlineMessage {
            id: hugeFontsMessage
            Layout.fillWidth: true
            position: Kirigami.InlineMessage.Position.Header
            type: Kirigami.MessageType.Warning
            showCloseButton: true
            text: i18n("Very large fonts may produce odd-looking results. Instead of using a very large font size, consider adjusting the global screen scale.")

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
            id: dpiTwiddledMessage
            Layout.fillWidth: true
            position: Kirigami.InlineMessage.Position.Header
            showCloseButton: true
            text: i18n("The recommended way to scale the user interface is using the global screen scaling feature.")
            actions: [ kscreenAction ]
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

    Kirigami.FormLayout {
        id: formLayout
        readonly property int maxImplicitWidth: Math.max(adjustAllFontsButton.implicitWidth, excludeField.implicitWidth, subPixelCombo.implicitWidth, hintingCombo.implicitWidth)

        QtControls.Button {
            id: adjustAllFontsButton
            Layout.preferredWidth: formLayout.maxImplicitWidth
            icon.name: "font-select-symbolic"
            text: i18n("&Adjust All Fonts…")

            onClicked: kcm.adjustAllFonts();
            enabled: !kcm.fontsSettings.isImmutable("font")
                    || !kcm.fontsSettings.isImmutable("fixed")
                    || !kcm.fontsSettings.isImmutable("smallestReadableFont")
                    || !kcm.fontsSettings.isImmutable("toolBarFont")
                    || !kcm.fontsSettings.isImmutable("menuFont")
                    || !kcm.fontsSettings.isImmutable("activeFont")
        }

        FontWidget {
            id: generalFontWidget
            label: i18n("General:")
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
            label: i18n("Fixed width:")
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
            label: i18n("Small:")
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
            label: i18n("Toolbar:")
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
            label: i18n("Menu:")
            tooltipText: i18n("Select menu font")
            category: "menuFont"
            font: kcm.fontsSettings.menuFont

            KCM.SettingStateBinding {
                configObject: kcm.fontsSettings
                settingName: "menuFont"
            }
        }
        FontWidget {
            label: i18n("Window title:")
            tooltipText: i18n("Select window title font")
            category: "activeFont"
            font: kcm.fontsSettings.activeFont

            KCM.SettingStateBinding {
                configObject: kcm.fontsSettings
                settingName: "activeFont"
            }
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
        }

        RowLayout {
            Kirigami.FormData.label: i18n("Anti-Aliasing:")
            QtControls.CheckBox {
                id: antiAliasingCheckBox
                checked: kcm.fontsAASettings.antiAliasing
                onToggled: kcm.fontsAASettings.antiAliasing = checked
                text: i18n("Enable")
                Layout.fillWidth: true
            }
            Kirigami.ContextualHelpButton {
                toolTipText: xi18nc("@info:tooltip Anti-Aliasing", "Pixels on displays are generally aligned in a grid. Therefore shapes of fonts that do not align with this grid will look blocky and wrong unless <emphasis>anti-aliasing</emphasis> techniques are used to reduce this effect. You generally want to keep this option enabled unless it causes problems.")
            }

            KCM.SettingStateBinding {
                configObject: kcm.fontsAASettings
                settingName: "antiAliasing"
                extraEnabledConditions: !kcm.fontsAASettings.isAaImmutable
            }
        }

        QtControls.CheckBox {
            id: excludeCheckBox
            checked: kcm.fontsAASettings.exclude
            onToggled: kcm.fontsAASettings.exclude = checked;
            text: i18n("Exclude range from anti-aliasing")
            Layout.fillWidth: true

            KCM.SettingStateBinding {
                configObject: kcm.fontsAASettings
                settingName: "exclude"
                extraEnabledConditions: !kcm.fontsAASettings.isAaImmutable && antiAliasingCheckBox.checked
            }
        }

        RowLayout {
            id: excludeField
            Layout.preferredWidth: formLayout.maxImplicitWidth
            enabled: antiAliasingCheckBox.enabled && antiAliasingCheckBox.checked

            QtControls.SpinBox {
                id: excludeFromSpinBox
                stepSize: 1
                onValueModified: kcm.fontsAASettings.excludeFrom = value
                textFromValue: function(value, locale) { return i18n("%1 pt", value)}
                valueFromText: function(text, locale) { return parseInt(text) }
                editable: true
                value: kcm.fontsAASettings.excludeFrom

                KCM.SettingStateBinding {
                    configObject: kcm.fontsAASettings
                    settingName: "excludeFrom"
                    extraEnabledConditions: excludeCheckBox.checked
                }
            }

            QtControls.Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: i18n("to")
                textFormat: Text.PlainText
                enabled: excludeCheckBox.checked
            }

            QtControls.SpinBox {
                id: excludeToSpinBox
                stepSize: 1
                onValueModified: kcm.fontsAASettings.excludeTo = value
                textFromValue: function(value, locale) { return i18n("%1 pt", value)}
                valueFromText: function(text, locale) { return parseInt(text) }
                editable: true
                value: kcm.fontsAASettings.excludeTo

                KCM.SettingStateBinding {
                    configObject: kcm.fontsAASettings
                    settingName: "excludeTo"
                    extraEnabledConditions: excludeCheckBox.checked
                }
            }
            Connections {
                target: kcm.fontsAASettings
                function onExcludeFromChanged() {
                    excludeFromSpinBox.value = kcm.fontsAASettings.excludeFrom;
                }
                function onExcludeToChanged() {
                    excludeToSpinBox.value = kcm.fontsAASettings.excludeTo;
                }
            }
        }

        RowLayout {
            Kirigami.FormData.label: i18nc("Used as a noun, and precedes a combobox full of options", "Sub-pixel rendering:")
            QtControls.ComboBox {
                id: subPixelCombo
                Layout.preferredWidth: formLayout.maxImplicitWidth
                currentIndex: kcm.subPixelCurrentIndex
                onActivated: (index) => {
                    kcm.subPixelCurrentIndex = index
                }
                model: kcm.subPixelOptionsModel
                textRole: "display"
                popup.height: popup.implicitHeight
                delegate: QtControls.ItemDelegate {
                    id: subPixelDelegate
                    width: Math.max(parent.width, implicitWidth)
                    onWidthChanged: {
                        subPixelCombo.popup.width = Math.max(subPixelCombo.popup.width, width)
                    }
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
                            source: "image://preview/" + model.index + "_" + kcm.hintingCurrentIndex + ".png"
                            // Setting sourceSize here is necessary as a workaround for QTBUG-38127
                            //
                            // With this bug, images requested from a QQuickImageProvider have an incorrect scale with devicePixelRatio != 1 when sourceSize is not set.
                            //
                            // TODO: Check if QTBUG-38127 is fixed and remove the next two lines.
                            sourceSize.width: 1
                            sourceSize.height: 1
                        }
                    }
                }

                KCM.SettingStateBinding {
                    configObject: kcm.fontsAASettings
                    settingName: "subPixel"
                    extraEnabledConditions: antiAliasingCheckBox.checked && !kcm.fontsAASettings.isAaImmutable
                }
            }
            Kirigami.ContextualHelpButton {
                toolTipText: xi18nc("@info:tooltip Sub-pixel rendering", "<para>On TFT or LCD screens every single pixel is actually composed of three or four smaller monochrome lights. These <emphasis>sub-pixels</emphasis> can be changed independently to further improve the quality of displayed fonts.</para> <para>The rendering quality is only improved if the selection matches the manner in which the sub-pixels of your display are aligned. Most displays have a linear ordering of <emphasis>RGB</emphasis> sub-pixels, some have <emphasis>BGR</emphasis> and some exotic orderings are not supported by this feature.</para>This does not work with CRT monitors.")
            }
        }

        RowLayout {
            Kirigami.FormData.label: i18nc("Used as a noun, and precedes a combobox full of options", "Hinting:")
            QtControls.ComboBox {
                id: hintingCombo
                Layout.preferredWidth: formLayout.maxImplicitWidth
                currentIndex: kcm.hintingCurrentIndex
                onActivated: (index) => {
                    kcm.hintingCurrentIndex = index
                }
                model: kcm.hintingOptionsModel
                textRole: "display"
                popup.height: popup.implicitHeight
                delegate: QtControls.ItemDelegate {
                    id: hintingDelegate
                    width: Math.max(parent.width, implicitWidth)
                    onWidthChanged: {
                        hintingCombo.popup.width = Math.max(hintingCombo.popup.width, width)
                    }
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
                            source: "image://preview/" + kcm.subPixelCurrentIndex + "_" + model.index + ".png"
                            // Setting sourceSize here is necessary as a workaround for QTBUG-38127
                            //
                            // With this bug, images requested from a QQuickImageProvider have an incorrect scale with devicePixelRatio != 1 when sourceSize is not set.
                            //
                            // TODO: Check if QTBUG-38127 is fixed and remove the next two lines.
                            sourceSize.width: 1
                            sourceSize.height: 1
                        }
                    }
                }
                KCM.SettingStateBinding {
                    configObject: kcm.fontsAASettings
                    settingName: "hinting"
                    extraEnabledConditions: antiAliasingCheckBox.checked && !kcm.fontsAASettings.isAaImmutable
                }
            }
            Kirigami.ContextualHelpButton {
                toolTipText: xi18nc("@info:tooltip Hinting", "Hinting is a technique in which hints embedded in a font are used to enhance the rendering quality especially at small sizes. Stronger hinting generally leads to sharper edges but the small letters will less closely resemble their shape at big sizes.")
            }
        }

        RowLayout {
            Layout.preferredWidth: formLayout.maxImplicitWidth
            // We don't want people messing with the font DPI on Wayland;
            // they should always be using the global scaling system instead
            visible: Qt.platform.pluginName === "xcb"

            QtControls.CheckBox {
                id: dpiCheckBox
                checked: kcm.fontsAASettings.dpi !== 0
                text: i18n("Force font DPI:")
                onToggled: {
                    kcm.fontsAASettings.dpi = checked ? dpiSpinBox.value : 0
                    dpiTwiddledMessage.visible = checked
                }

                KCM.SettingStateBinding {
                    configObject: kcm.fontsAASettings
                    settingName: "forceFontDPI"
                    extraEnabledConditions: antiAliasingCheckBox.checked && !kcm.fontsAASettings.isAaImmutable
                }
            }

            QtControls.SpinBox {
                id: dpiSpinBox
                editable: true
                value: kcm.fontsAASettings.dpi !== 0 ? kcm.fontsAASettings.dpi : 96
                onValueModified: kcm.fontsAASettings.dpi = value
                to: 999
                from: 1

                KCM.SettingStateBinding {
                    configObject: kcm.fontsAASettings
                    settingName: "forceFontDPI"
                    extraEnabledConditions: dpiCheckBox.enabled && dpiCheckBox.checked
                }
            }
            Kirigami.ContextualHelpButton {
                toolTipText: xi18nc("@info:tooltip Force fonts DPI", "<para>Enter your screen's DPI here to make on-screen fonts match their physical sizes when printed. Changing this option from its default value will conflict with many apps; some icons and images may not scale as expected.</para><para>To increase text size, change the size of the fonts above. To scale everything, use the scaling slider on the <interface>Display & Monitor</interface> page.</para>")
            }
        }
    }
}
