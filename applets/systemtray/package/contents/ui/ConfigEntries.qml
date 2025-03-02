/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2019 Konrad Materka <materka@gmail.com>
    SPDX-FileCopyrightText: 2022 ivan (@ratijas) tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kcmutils as KCMUtils
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels
import org.kde.kquickcontrols as KQC
import org.kde.plasma.plasmoid

KCMUtils.ScrollViewKCM {
    id: iconsPage

    signal configurationChanged

    property var cfg_shownItems: []
    property var cfg_hiddenItems: []
    property var cfg_extraItems: []
    property var cfg_disabledStatusNotifiers: []
    property alias cfg_showAllItems: showAllCheckBox.checked

    function categoryName(category) {
        switch (category) {
        case "ApplicationStatus":
            return i18n("Application Status")
        case "Communications":
            return i18n("Communications")
        case "SystemServices":
            return i18n("System Services")
        case "Hardware":
            return i18n("Hardware Control")
        case "UnknownCategory":
        default:
            return i18n("Miscellaneous")
        }
    }

    // Frameless style for the InlineMessage is not visually compatible here
    header: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        Kirigami.SearchField {
            Layout.fillWidth: true
            id: filterField
        }

        Kirigami.InlineMessage {
            id: disablingSniMessage
            property string appName

            function showWithAppName(appName: string) {
                disablingSniMessage.appName = appName
                visible = true;
            }

            Layout.fillWidth: true
            type: Kirigami.MessageType.Warning
            text: xi18nc("@info:usagetip", "Look for a setting in <application>%1</application> to disable its tray icon before doing it here. Some apps’ tray icons were not designed to be disabled, and using this setting may cause them to behave unexpectedly.<nl/><nl/>Use this setting at your own risk, and do not report issues to KDE or the app’s author.", appName)
            actions: [
                Kirigami.Action {
                    text: i18nc("@action:button", "I understand the risks")
                    onTriggered: disablingSniMessage.visible = false;
                }
            ]
        }
    }

    view: ListView {
        id: itemsList

        property real visibilityColumnWidth: Kirigami.Units.gridUnit
        property real keySequenceColumnWidth: Kirigami.Units.gridUnit
        readonly property int iconSize: Kirigami.Units.iconSizes.smallMedium

        clip: true

        model: KItemModels.KSortFilterProxyModel {
            filterString: filterField.text
            filterCaseSensitivity: Qt.CaseInsensitive
            Component.onCompleted: sourceModel = Plasmoid.configSystemTrayModel // avoid unnecessary binding, it causes loops
        }
        reuseItems: true

        header: RowLayout {
            width: itemsList.width
            spacing: Kirigami.Units.smallSpacing

            Item {
                implicitWidth: itemsList.iconSize + 2 * Kirigami.Units.smallSpacing
            }
            Kirigami.Heading {
                text: i18nc("Name of the system tray entry", "Entry")
                textFormat: Text.PlainText
                level: 2
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Kirigami.Heading {
                text: i18n("Visibility")
                textFormat: Text.PlainText
                level: 2
                Layout.preferredWidth: itemsList.visibilityColumnWidth
                Component.onCompleted: itemsList.visibilityColumnWidth = Math.max(implicitWidth, itemsList.visibilityColumnWidth)
            }
            Kirigami.Heading {
                text: i18n("Keyboard Shortcut")
                textFormat: Text.PlainText
                level: 2
                Layout.preferredWidth: itemsList.keySequenceColumnWidth
                Component.onCompleted: itemsList.keySequenceColumnWidth = Math.max(implicitWidth, itemsList.keySequenceColumnWidth)
            }
            QQC2.Button { // Configure button column
                icon.name: "configure"
                enabled: false
                opacity: 0
                Layout.rightMargin: 2 * Kirigami.Units.smallSpacing
            }
        }

        section {
            property: "category"
            delegate: Kirigami.ListSectionHeader {
                label: categoryName(section)
                width: itemsList.width
            }
        }

        delegate: QQC2.ItemDelegate {
            id: listItem

            width: itemsList.width

            Kirigami.Theme.useAlternateBackgroundColor: true

            // Don't need highlight, hover, or pressed effects
            highlighted: false
            hoverEnabled: false
            down: false

            readonly property bool isPlasmoid: model.itemType === "Plasmoid"

            contentItem: FocusScope {
                implicitHeight: childrenRect.height

                onActiveFocusChanged: if (activeFocus) {
                    listItem.ListView.view.positionViewAtIndex(index, ListView.Contain);
                }

                RowLayout {
                    width: parent.width
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Icon {
                        implicitWidth: itemsList.iconSize
                        implicitHeight: itemsList.iconSize
                        source: model.decoration
                        animated: false
                    }

                    QQC2.Label {
                        id: nameLabel
                        Layout.fillWidth: true
                        text: model.display
                        textFormat: Text.PlainText
                        elide: Text.ElideRight

                        QQC2.ToolTip {
                            visible: listItem.hovered && parent.truncated
                            text: parent.text
                        }
                    }

                    QQC2.ComboBox {
                        id: visibilityComboBox

                        property real contentWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                                                            implicitContentWidth + leftPadding + rightPadding)
                        readonly property bool shown:    cfg_showAllItems || cfg_shownItems.indexOf(itemId) !== -1
                        readonly property bool hidden:   cfg_hiddenItems.indexOf(itemId) !== -1
                        readonly property bool disabled: (isPlasmoid && cfg_extraItems.indexOf(itemId) === -1)
                                                      || (!isPlasmoid && cfg_disabledStatusNotifiers.indexOf(itemId) > -1)

                        implicitWidth: Math.max(contentWidth, itemsList.visibilityColumnWidth)
                        Component.onCompleted: itemsList.visibilityColumnWidth = Math.max(implicitWidth, itemsList.visibilityColumnWidth)

                        enabled: !cfg_showAllItems && itemId
                        textRole: "text"
                        valueRole: "value"
                        model: comboBoxModel()

                        currentIndex: {
                            let value

                            if (shown) {
                                value = "shown"
                            } else if (hidden) {
                                value = "hidden"
                            } else if (disabled) {
                                value = "disabled"
                            } else {
                                value = "auto"
                            }

                            for (let i = 0; i < model.length; i++) {
                                if (model[i].value === value) {
                                    return i
                                }
                            }

                            return 0
                        }

                        onActivated: index => {
                            const shownIndex = cfg_shownItems.indexOf(itemId)
                            const hiddenIndex = cfg_hiddenItems.indexOf(itemId)
                            const extraIndex = cfg_extraItems.indexOf(itemId)
                            const disabledSniIndex = cfg_disabledStatusNotifiers.indexOf(itemId)

                            switch (currentValue) {
                            case "auto":
                                if (shownIndex > -1) {
                                    cfg_shownItems.splice(shownIndex, 1)
                                }
                                if (hiddenIndex > -1) {
                                    cfg_hiddenItems.splice(hiddenIndex, 1)
                                }
                                if (isPlasmoid && extraIndex === -1) {
                                    cfg_extraItems.push(itemId)
                                } else if (!isPlasmoid && disabledSniIndex > -1) {
                                    cfg_disabledStatusNotifiers.splice(disabledSniIndex, 1)
                                }
                                break
                            case "shown":
                                if (shownIndex === -1) {
                                    cfg_shownItems.push(itemId)
                                }
                                if (hiddenIndex > -1) {
                                    cfg_hiddenItems.splice(hiddenIndex, 1)
                                }
                                if (isPlasmoid && extraIndex === -1) {
                                    cfg_extraItems.push(itemId)
                                } else if (!isPlasmoid && disabledSniIndex > -1) {
                                    cfg_disabledStatusNotifiers.splice(disabledSniIndex, 1)
                                }
                                break
                            case "hidden":
                                if (shownIndex > -1) {
                                    cfg_shownItems.splice(shownIndex, 1)
                                }
                                if (hiddenIndex === -1) {
                                    cfg_hiddenItems.push(itemId)
                                }
                                if (isPlasmoid && extraIndex === -1) {
                                    cfg_extraItems.push(itemId)
                                } else if (!isPlasmoid && disabledSniIndex > -1) {
                                    cfg_disabledStatusNotifiers.splice(disabledSniIndex, 1)
                                }
                                break
                            case "disabled":
                                if (shownIndex > -1) {
                                    cfg_shownItems.splice(shownIndex, 1)
                                }
                                if (hiddenIndex > -1) {
                                    cfg_hiddenItems.splice(hiddenIndex, 1)
                                }
                                if (isPlasmoid && extraIndex >= -1) {
                                    cfg_extraItems.splice(extraIndex, 1)
                                } else if (!isPlasmoid && disabledSniIndex === -1) {
                                    disablingSniMessage.showWithAppName(nameLabel.text)
                                    cfg_disabledStatusNotifiers.push(itemId)
                                }
                                break
                            }
                            iconsPage.configurationChanged()
                        }

                        function comboBoxModel() {
                            const autoElement = {"value": "auto", "text": i18n("Shown when relevant")}
                            const shownElement = {"value": "shown", "text": i18n("Always shown")}
                            const hiddenElement = {"value": "hidden", "text": i18n("Always hidden")}
                            const disabledElement = {"value": "disabled", "text": i18n("Disabled")}

                            if (showAllCheckBox.checked) {
                                return [autoElement, disabledElement]
                            } else {
                                return [autoElement, shownElement, hiddenElement, disabledElement]
                            }
                        }
                    }
                    KQC.KeySequenceItem {
                        id: keySequenceItem
                        Layout.minimumWidth: itemsList.keySequenceColumnWidth
                        Layout.preferredWidth: itemsList.keySequenceColumnWidth
                        Component.onCompleted: itemsList.keySequenceColumnWidth = Math.max(implicitWidth, itemsList.keySequenceColumnWidth)

                        visible: isPlasmoid
                        enabled: visibilityComboBox.currentValue !== "disabled"
                        keySequence: model.applet ? model.applet.plasmoid.globalShortcut : ""
                        onCaptureFinished: {
                            if (model.applet && keySequence !== model.applet.plasmoid.globalShortcut) {
                                model.applet.plasmoid.globalShortcut = keySequence

                                itemsList.keySequenceColumnWidth = Math.max(implicitWidth, itemsList.keySequenceColumnWidth)
                            }
                        }
                    }
                    // Placeholder for when KeySequenceItem is not visible
                    Item {
                        Layout.minimumWidth: itemsList.keySequenceColumnWidth
                        Layout.maximumWidth: itemsList.keySequenceColumnWidth
                        visible: !keySequenceItem.visible
                    }

                    QQC2.Button {
                        readonly property QtObject configureAction: (model.applet && model.applet.plasmoid.internalAction("configure")) || null

                        Accessible.name: configureAction ? configureAction.text : ""
                        icon.name: "configure"
                        enabled: configureAction && configureAction.visible && configureAction.enabled
                        // Still reserve layout space, so not setting visible to false
                        opacity: enabled ? 1 : 0
                        onClicked: configureAction.trigger()

                        QQC2.ToolTip {
                            // Strip out ampersands right before non-whitespace characters, i.e.
                            // those used to determine the alt key shortcut
                            text: parent.Accessible.name.replace(/&(?=\S)/g, "")
                        }
                    }
                }
            }
        }
    }

    // Re-add separator line between footer and list view
    extraFooterTopPadding: true
    footer: QQC2.CheckBox {
        id: showAllCheckBox
        text: i18n("Always show all entries")
        Layout.alignment: Qt.AlignVCenter
    }
}
