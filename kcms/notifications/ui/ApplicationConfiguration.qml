/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.15
import QtQml.Models 2.2
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Dialogs

import org.kde.kirigami 2.20 as Kirigami
import org.kde.kcmutils as KCM

import org.kde.private.kcms.notifications 1.0 as Private

ColumnLayout {
    id: configColumn

    property var rootIndex
    property var behaviorSettings: rootIndex ? kcm.behaviorSettings(rootIndex) : null

    readonly property string otherAppsId: "@other"

    readonly property string appDisplayName: rootIndex ? kcm.sourcesModel.data(rootIndex, Qt.DisplayRole) || "" : ""
    readonly property string appIconName: rootIndex ? kcm.sourcesModel.data(rootIndex, Qt.DecorationRole) || "" : ""
    readonly property string desktopEntry: rootIndex ? kcm.sourcesModel.data(rootIndex, Private.SourcesModel.DesktopEntryRole) || "" : ""

    onRootIndexChanged: {
        kcm.eventsModel.rootIndex = rootIndex
    }

    spacing: 0

    // Top content
    Rectangle {
        Layout.fillWidth: true
        implicitHeight: childrenRect.height

        Kirigami.Theme.colorSet: Kirigami.Theme.Window
        Kirigami.Theme.inherit: false
        color: Kirigami.Theme.backgroundColor

        ColumnLayout {
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
            }

            spacing: 0

            // App name and controls
            ColumnLayout {
                readonly property int marginsAndSpacings: Kirigami.Units.largeSpacing

                Layout.fillWidth: true
                Layout.margins: marginsAndSpacings

                spacing: marginsAndSpacings

                // App icon/name header
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter

                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Icon {
                        Layout.preferredWidth: Kirigami.Units.iconSizes.medium
                        Layout.preferredHeight: Kirigami.Units.iconSizes.medium
                        source: configColumn.appIconName
                    }

                    Kirigami.Heading {
                        level: 2
                        horizontalAlignment: Text.AlignLeft
                        text: configColumn.appDisplayName
                        elide: Text.ElideRight
                        textFormat: Text.PlainText
                    }
                }

                // Controls
                Kirigami.FormLayout {
                    Layout.fillWidth: true

                    QQC2.CheckBox {
                        id: showPopupsCheck
                        text: i18n("Show popups")
                        checked: !!behaviorSettings && behaviorSettings.showPopups
                        onClicked: behaviorSettings.showPopups = checked

                        KCM.SettingStateBinding {
                            configObject: behaviorSettings
                            settingName: "ShowPopups"
                        }
                    }

                    RowLayout { // just for indentation
                        QQC2.CheckBox {
                            Layout.leftMargin: mirrored ? 0 : indicator.width
                            Layout.rightMargin: mirrored ? indicator.width : 0
                            text: i18n("Show in do not disturb mode")
                            checked: !!behaviorSettings && behaviorSettings.showPopupsInDndMode
                            onClicked: behaviorSettings.showPopupsInDndMode = checked

                            KCM.SettingStateBinding {
                                configObject: behaviorSettings
                                settingName: "ShowPopupsInDndMode"
                                extraEnabledConditions: showPopupsCheck.checked
                            }
                        }
                    }

                    QQC2.CheckBox {
                        text: i18n("Show in history")
                        checked: !!behaviorSettings && behaviorSettings.showInHistory
                        onClicked: behaviorSettings.showInHistory = checked

                        KCM.SettingStateBinding {
                            configObject: behaviorSettings
                            settingName: "ShowInHistory"
                        }
                    }

                    QQC2.CheckBox {
                        text: i18n("Show notification badges")
                        checked: !!behaviorSettings && behaviorSettings.showBadges
                        onClicked: behaviorSettings.showBadges = checked

                        KCM.SettingStateBinding {
                            configObject: behaviorSettings
                            settingName: "ShowBadges"
                            extraEnabledConditions: !!configColumn.desktopEntry && configColumn.desktopEntry !== configColumn.otherAppsId
                        }
                    }
                }
            }
        }
    }

    Kirigami.Separator {
        Layout.fillWidth: true
    }

    // Per-events view header
    Rectangle {
        Layout.fillWidth: true
        implicitHeight: tableHeaderText.implicitHeight + (2 * tableHeaderText.anchors.topMargin)

        Kirigami.Theme.colorSet: Kirigami.Theme.View
        Kirigami.Theme.inherit: false
        // We want a color that's basically halfway between the view background
        // color and the window background color. But due to the use of color
        // scopes, only one will be available at a time. So to get basically the
        // same thing, we blend the view background color with a smidgen of the
        // text color.
        color: Qt.tint(Kirigami.Theme.backgroundColor,
                    Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.03))

        visible: eventsList.count > 0

        Kirigami.Heading {
            id: tableHeaderText

            anchors {
                left: parent.left
                leftMargin: Kirigami.Units.largeSpacing
                right: parent.right
                rightMargin: Kirigami.Units.smallSpacing
                top: parent.top
                topMargin: Kirigami.Units.smallSpacing
            }

            level: 2
            text: i18nc("@title:table Configure individual notification events in an app", "Configure Events")
        }
    }

    Kirigami.Separator {
        Layout.fillWidth: true
        visible: eventsList.count > 0
    }

    // Per-events view
    QQC2.ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true

        Kirigami.Theme.colorSet: Kirigami.Theme.View
        Kirigami.Theme.inherit: false

        contentItem: ListView {
            id: eventsList

            headerPositioning: ListView.OverlayHeader

            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                anchors.verticalCenterOffset: eventsList.headerItem.height/2
                width: parent.width - Kirigami.Units.largeSpacing * 4
                text: i18n("This application does not support configuring notifications on a per-event basis");
                visible: eventsList.count === 0
            }

            model: kcm.eventsModel
            delegate: Kirigami.AbstractListItem {
                id: eventDelegate

                property bool expanded: false

                alternatingBackground: true
                Keys.forwardTo: expandButton
                down: false

                ColumnLayout {
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop

                        Kirigami.Icon {
                            Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                            Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium

                            visible: model.showIcons
                            source: model.decoration
                        }

                        QQC2.Label {
                            Layout.fillWidth: true
                            text: model.display
                            elide: Text.ElideRight
                        }

                        Rectangle {
                            id: defaultIndicator
                            radius: width * 0.5
                            implicitWidth: Kirigami.Units.largeSpacing
                            implicitHeight: Kirigami.Units.largeSpacing
                            Layout.rightMargin: Kirigami.Units.smallSpacing
                            visible: kcm.defaultsIndicatorsVisible
                            opacity: !model.isDefault
                            color: Kirigami.Theme.neutralTextColor
                        }

                        Kirigami.Icon {
                            Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                            Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium

                            source: "dialog-information"
                            opacity: model.actions.includes("Popup") ? 1 : 0.2
                        }

                        Kirigami.Icon {
                            Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                            Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium

                            source: "audio-speakers-symbolic"
                            opacity: model.actions.includes("Sound") ? 1 : 0.2
                        }

                        QQC2.ToolButton {
                            id: expandButton
                            Layout.alignment: Qt.AlignRight
                            icon.name: eventDelegate.expanded ? "arrow-down" : "arrow-right"
                            onClicked: eventDelegate.expanded = !eventDelegate.expanded
                        }
                    }

                    ColumnLayout {
                        visible: eventDelegate.expanded

                        Layout.fillWidth: true
                        Layout.leftMargin: Kirigami.Units.largeSpacing
                        spacing: Kirigami.Units.smallSpacing

                        Kirigami.Heading {
                            Layout.fillWidth: true
                            Layout.bottomMargin: Kirigami.Units.smallSpacing
                            visible: model.comment
                            level: 5
                            opacity: 0.7
                            text: model.comment
                            wrapMode: Text.WordWrap
                        }

                        ActionCheckBox {
                            actionName: "Popup"
                            text: i18n("Show a message in a pop-up")
                        }

                        RowLayout {
                            id: soundRow

                            ActionCheckBox {
                                id: soundCheckBox
                                actionName: "Sound"
                                text: i18n("Play a sound")
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            QQC2.Button {
                                enabled: soundCheckBox.checked && model.sound
                                icon.name: "media-playback-start"
                                Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                                onClicked: kcm.playSound(model.sound)
                            }
                            QQC2.TextField {
                                enabled: soundCheckBox.checked
                                text: model.sound
                                onEditingFinished: model.sound = text

                                KCM.SettingHighlighter {
                                    highlight: model.sound !== model.defaultSound
                                }
                            }
                            QQC2.Button {
                                enabled: soundCheckBox.checked && model.sound !== model.defaultSound
                                icon.name: "edit-reset"
                                Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                                onClicked: model.sound = model.defaultSound
                            }
                            QQC2.Button {
                                enabled: soundCheckBox.checked
                                icon.name: "document-open-folder"
                                Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                                onClicked: {
                                    soundSelector.eventIndex = model.index
                                    soundSelector.open()
                                }
                            }
                        }

                        component ActionCheckBox : QQC2.CheckBox {
                            required property string actionName

                            checked: model.actions.includes(actionName)
                            onToggled: {
                                const _actions = model.actions
                                const idx = model.actions.indexOf(actionName)
                                if (checked && idx === -1 ) {
                                    _actions.push(actionName)
                                } else if (!checked && idx > -1) {
                                    _actions.splice(idx, 1) // remove actionName
                                } else {
                                    return // no changes needed
                                }
                                model.actions = _actions.sort() // Sort to make the list independent of the selection order
                            }

                            KCM.SettingHighlighter {
                                highlight: checked !== model.defaultActions.includes(actionName)
                            }
                        }
                    }

                    TapHandler {
                        onTapped: eventDelegate.expanded = !eventDelegate.expanded
                    }
                }

                Behavior on height {
                    NumberAnimation {
                        properties: "height"
                        duration: Kirigami.Units.shortDuration
                    }
                }
            }
        }
    }

    FileDialog {
        id: soundSelector

        property var eventIndex

        currentFolder: kcm.soundsLocation()
        nameFilters: ["Sound files(*.ogg *.oga *.wav)"]

        onAccepted: {
            kcm.eventsModel.setData(kcm.eventsModel.index(eventIndex, 0), selectedFile,  Private.SourcesModel.SoundRole)
        }
    }
}
