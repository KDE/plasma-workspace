/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.15
import QtQml.Models 2.2
import QtQuick.Layouts 1.15
import QtQuick.Controls as QQC2
import QtQuick.Dialogs

import org.kde.kirigami 2.20 as Kirigami
import org.kde.kcmutils as KCM

import org.kde.private.kcms.notifications 1.0 as Private

ColumnLayout {
    id: configColumn

    property alias rootIndex: eventsModel.rootIndex
    property var behaviorSettings: rootIndex ? kcm.behaviorSettings(rootIndex) : null
    property bool showOnlyEventsConfig: false

    readonly property string otherAppsId: "@other"
    readonly property bool eventsConfigVisible: desktopEntry !== otherAppsId

    readonly property string appDisplayName: rootIndex ? kcm.sourcesModel.data(rootIndex, Qt.DisplayRole) || "" : ""
    readonly property string appIconName: rootIndex ? kcm.sourcesModel.data(rootIndex, Qt.DecorationRole) || "" : ""
    readonly property string desktopEntry: rootIndex ? kcm.sourcesModel.data(rootIndex, Private.SourcesModel.DesktopEntryRole) || "" : ""

    spacing: 0

    function configureEvents(eventId: string): void {
        const idx = kcm.sourcesModel.indexOfEvent(eventsModel.rootIndex, eventId);
        if (!idx.valid) {
            return;
        }
        eventsList.currentIndex = idx.row;
        eventsList.positionViewAtIndex(idx.row, ListView.Contain);
        eventsList.forceActiveFocus();
        // This should be enough so that `currentItem` exists and it is updated to `currentIndex`
        // TODO: But we could make it actually fail-safe by binding it on the component.
        Qt.callLater(() => { eventsList.currentItem.expanded = true; });
    }

    // Top content
    Rectangle {
        Layout.fillWidth: true
        // Ensure background spans entire height when events config isn't shown.
        Layout.fillHeight: !configColumn.eventsConfigVisible
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
                visible: !configColumn.showOnlyEventsConfig

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
        visible: configColumn.eventsConfigVisible
    }

    // Per-events view
    QQC2.ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        visible: configColumn.eventsConfigVisible

        Kirigami.Theme.colorSet: Kirigami.Theme.View
        Kirigami.Theme.inherit: false

        contentItem: ListView {
            id: eventsList

            clip: true

            headerPositioning: ListView.OverlayHeader
            header: Kirigami.InlineViewHeader {
                width: eventsList.width
                text: i18nc("@title:table Configure individual notification events in an app", "Configure Events")
                visible: eventsList.count > 0
            }

            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                anchors.verticalCenterOffset: eventsList.headerItem.height/2
                width: parent.width - Kirigami.Units.largeSpacing * 4
                text: i18n("This application does not support configuring notifications on a per-event basis");
                visible: eventsList.count === 0
            }

            model: DelegateModel {
                id: eventsModel
                model: kcm.sourcesModel

                delegate: QQC2.ItemDelegate {
                    id: eventDelegate

                    property bool expanded: false

                    width: eventsList.width

                    Keys.forwardTo: expandButton
                    down: false
                    Kirigami.Theme.useAlternateBackgroundColor: true

                    contentItem: ColumnLayout {
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop

                            Kirigami.Icon {
                                Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                                Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium

                                visible: model?.showIcons ?? false
                                source: model?.decoration ?? ""
                            }

                            QQC2.Label {
                                Layout.fillWidth: true
                                text: model?.display ?? ""
                                textFormat: Text.PlainText
                                elide: Text.ElideRight
                            }

                            Rectangle {
                                id: defaultIndicator
                                radius: width * 0.5
                                implicitWidth: Kirigami.Units.largeSpacing
                                implicitHeight: Kirigami.Units.largeSpacing
                                Layout.rightMargin: Kirigami.Units.smallSpacing
                                visible: kcm.defaultsIndicatorsVisible
                                opacity: (model?.isDefault ?? true) ? 0 : 1
                                color: Kirigami.Theme.neutralTextColor
                            }

                            Kirigami.Icon {
                                Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                                Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium

                                source: "dialog-information"
                                opacity: model?.actions?.includes("Popup") ? 1 : 0.2
                            }

                            Kirigami.Icon {
                                Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                                Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium

                                source: "audio-speakers-symbolic"
                                opacity: model?.actions?.includes("Sound") ? 1 : 0.2
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
                                visible: text.length > 0
                                level: 5
                                opacity: 0.75
                                text: model?.comment ?? ""
                                textFormat: Text.PlainText
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
                                    enabled: soundCheckBox.checked && model && model.sound.length > 0
                                    display: QQC2.AbstractButton.IconOnly
                                    icon.name: "media-playback-start"
                                    text: i18nc("@info:tooltip", "Preview sound")

                                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                                    QQC2.ToolTip.text: text
                                    QQC2.ToolTip.visible: hovered || activeFocus

                                    onClicked: kcm.playSound(model.sound)
                                }
                                QQC2.TextField {
                                    enabled: soundCheckBox.checked
                                    text: model?.sound ?? ""
                                    onEditingFinished: model.sound = text

                                    // Make the TextField able to shrink, but keep its implicit width when is not needed
                                    Layout.fillWidth: true
                                    Layout.maximumWidth: implicitWidth
                                    Layout.minimumWidth: Kirigami.Units.gridUnit * 5

                                    KCM.SettingHighlighter {
                                        highlight: model?.sound !== model?.defaultSound
                                    }
                                }
                                QQC2.Button {
                                    enabled: soundCheckBox.checked && model?.sound !== model?.defaultSound
                                    display: QQC2.AbstractButton.IconOnly
                                    icon.name: "edit-reset"
                                    text: i18nc("Reset the notification sound to a default one", "Reset")

                                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                                    QQC2.ToolTip.text: text
                                    QQC2.ToolTip.visible: hovered || activeFocus

                                    onClicked: model.sound = model.defaultSound
                                }
                                QQC2.Button {
                                    enabled: soundCheckBox.checked
                                    display: QQC2.AbstractButton.IconOnly
                                    icon.name: "document-open-folder"
                                    text: i18n("Choose sound file")

                                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                                    QQC2.ToolTip.text: text
                                    QQC2.ToolTip.visible: hovered || activeFocus

                                    onClicked: {
                                        soundSelector.eventRow = model.index
                                        soundSelector.open()
                                    }
                                }
                            }

                            component ActionCheckBox : QQC2.CheckBox {
                                required property string actionName

                                checked: model?.actions?.includes(actionName) ?? false
                                onToggled: {
                                    const _actions = model.actions
                                    const idx = _actions.indexOf(actionName)
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
                                    highlight: checked !== model?.defaultActions?.includes(actionName)
                                }
                            }
                        }
                    }

                    TapHandler {
                        onTapped: {
                            eventsList.currentIndex = model.index;
                            forceActiveFocus();
                            eventDelegate.expanded = true;
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
    }

    FileDialog {
        id: soundSelector

        property var eventRow

        currentFolder: kcm.soundsLocation()
        nameFilters: ["Sound files(*.ogg *.oga *.wav)"]

        onAccepted: {
            kcm.sourcesModel.setData(kcm.sourcesModel.index(eventRow, 0, eventsModel.rootIndex), selectedFile, Private.SourcesModel.SoundRole)
        }
    }
}
