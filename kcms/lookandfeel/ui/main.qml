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
import org.kde.kitemmodels as KItemModels
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

            LookAndFeelThumbnail {
                id: lightLookAndFeelRadioButton
                preview: lightLookAndFeelInformation.preview
                text: i18nc("@option:radio Light global theme", "Light")
                checked: selectedLookAndFeelInformation.variant !== "dark"
                QtControls.ButtonGroup.group: themeGroup

                onClicked: {
                    kcm.settings.lookAndFeelPackage = kcm.settings.defaultLightLookAndFeel;
                }
            }

            LookAndFeelThumbnail {
                id: darkLookAndFeelRadioButton
                preview: darkLookAndFeelInformation.preview
                text: i18nc("@option:radio Dark global theme", "Dark")
                checked: selectedLookAndFeelInformation.variant === "dark"
                QtControls.ButtonGroup.group: themeGroup

                onClicked: {
                    kcm.settings.lookAndFeelPackage = kcm.settings.defaultDarkLookAndFeel;
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

        KCM.GridView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            view.currentIndex: selectedLookAndFeelRow.index
            view.model: lookAndFeelPackages

            view.delegate: KCM.GridDelegate {
                id: delegate

                text: model.display
                subtitle: model.hasDesktopLayout ? i18nc("@label", "Contains Desktop layout") : ""
                toolTip: model.description

                thumbnailAvailable: model.screenshot
                thumbnail: Image {
                    anchors.fill: parent
                    source: model.screenshot || ""
                    sourceSize: Qt.size(delegate.GridView.view.cellWidth * Screen.devicePixelRatio,
                                        delegate.GridView.view.cellHeight * Screen.devicePixelRatio)
                }
                actions: [
                    Kirigami.Action {
                        visible: model.fullScreenPreview !== ""
                        icon.name: "view-preview"
                        tooltip: i18nc("@info:tooltip", "Preview Theme")
                        onTriggered: {
                            previewWindow.url = "file:/" + model.fullScreenPreview
                            previewWindow.showFullScreen()
                        }
                    },
                    Kirigami.Action {
                        icon.name: "edit-delete"
                        tooltip: if (enabled) {
                            return i18nc("@info:tooltip", "Remove theme");
                        } else if (delegate.GridView.isCurrentItem) {
                            return i18nc("@info:tooltip", "Cannot delete the active theme");
                        } else {
                            return i18nc("@info:tooltip", "Cannot delete system-installed themes");
                        }
                        enabled: model.uninstallable && !delegate.GridView.isCurrentItem
                        onTriggered: confirmDeletionDialog.incubateObject(root, {
                            "pluginId": model.pluginName,
                        });
                    }
                ]
                onClicked: {
                    kcm.settings.lookAndFeelPackage = model.pluginName;
                    if (lightLookAndFeelRadioButton.checked) {
                        kcm.settings.defaultLightLookAndFeel = model.pluginName;
                    } else if (darkLookAndFeelRadioButton.checked) {
                        kcm.settings.defaultDarkLookAndFeel = model.pluginName;
                    }
                }
            }

            KItemModels.KSortFilterProxyModel {
                id: lookAndFeelPackages
                sourceModel: kcm.model
                filterRoleName: "variant"
                filterRowCallback: {
                    const variant = selectedLookAndFeelInformation.variant;
                    return (sourceRow, sourceParent) => {
                        let value = sourceModel.data(sourceModel.index(sourceRow, 0, sourceParent), filterRole);
                        if (value === "") {
                            return true;
                        } else {
                            return value === variant;
                        }
                    };
                }
            }

            Private.ItemModelRow {
                id: selectedLookAndFeelRow
                model: lookAndFeelPackages
                role: "pluginName"
                value: kcm.settings.lookAndFeelPackage
            }
        }
    }

    Component {
        id: confirmDeletionDialog

        Kirigami.PromptDialog {
            id: dialog
            required property string pluginId

            parent: root
            title: i18nc("@title:window", "Delete Permanently")
            subtitle: i18nc("@label", "Do you really want to permanently delete this theme?")
            standardButtons: Kirigami.Dialog.NoButton
            customFooterActions: [
                Kirigami.Action {
                    text: i18nc("@action:button", "Delete Permanently")
                    icon.name: "delete"
                    onTriggered: dialog.accept();
                },
                Kirigami.Action {
                    text: i18nc("@action:button", "Cancel")
                    icon.name: "dialog-cancel"
                    onTriggered: dialog.reject();
                }
            ]

            onAccepted: kcm.removeRow(pluginId, true);
            onClosed: destroy();

            Component.onCompleted: open();
        }
    }

    Window {
        id: previewWindow
        property alias url: previewImage.source
        color: Qt.rgba(0, 0, 0, 0.7)
        MouseArea {
            anchors.fill: parent
            Image {
                id: previewImage
                anchors.centerIn: parent
                fillMode: Image.PreserveAspectFit
                width: Math.min(parent.width, sourceSize.width)
                height: Math.min(parent.height, sourceSize.height)
            }
            onClicked: previewWindow.close()
            QtControls.ToolButton {
                anchors {
                    top: parent.top
                    right: parent.right
                }
                icon.name: "window-close"
                onClicked: previewWindow.close()
            }
            Shortcut {
                onActivated: previewWindow.close()
                sequence: "Esc"
            }
        }
    }
}
