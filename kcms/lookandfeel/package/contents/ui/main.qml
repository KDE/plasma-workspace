/*
    SPDX-FileCopyrightText: 2018 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

import QtQuick 2.6
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import QtQuick.Controls 2.3 as QtControls
import org.kde.kirigami 2.5 as Kirigami
import org.kde.newstuff 1.62 as NewStuff
import org.kde.kconfig 1.0 // for KAuthorized
import org.kde.kcm 1.3 as KCM

KCM.GridViewKCM {
    id: root
    KCM.ConfigModule.quickHelp: i18n("This module lets you choose the global look and feel.")

    view.model: kcm.lookAndFeelModel
    view.currentIndex: kcm.pluginIndex(kcm.lookAndFeelSettings.lookAndFeelPackage)

    KCM.SettingStateBinding {
        configObject: kcm.lookAndFeelSettings
        settingName: "lookAndFeelPackage"
    }

    view.delegate: KCM.GridDelegate {
        id: delegate

        text: model.display
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
                iconName: "view-preview"
                tooltip: i18n("Preview Theme")
                onTriggered: {
                    previewWindow.url = model.fullScreenPreview;
                    previewWindow.showFullScreen();
                }
            }
        ]
        onClicked: {
            kcm.lookAndFeelSettings.lookAndFeelPackage = model.pluginName;
            view.forceActiveFocus();
            kcm.resetDefaultLayout = false;
        }
        onDoubleClicked: {
            kcm.save();
        }
    }

    footer: ColumnLayout {
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            type: Kirigami.MessageType.Warning
            text: i18n("Your current layout and configuration of panels, desktop widgets, and wallpapers will be lost and reset to the default layout provided by the selected theme.")
            visible: resetCheckbox.checked
        }

        RowLayout {
            Layout.fillWidth: true

            QtControls.CheckBox {
                id: resetCheckbox
                checked: kcm.resetDefaultLayout
                text: i18n("Use desktop layout from theme")
                onCheckedChanged: kcm.resetDefaultLayout = checked;
            }

            Kirigami.ActionToolBar {
                flat: false
                alignment: Qt.AlignRight
                actions: [
                    Kirigami.Action {
                        text: i18n("Get New Global Themes…")
                        icon.name: "get-hot-new-stuff"
                        onTriggered: { newStuffPage.open(); }
                    }
                ]
            }
        }
    }

    Loader {
        id: newStuffPage

        // Use this function to open the dialog. It seems roundabout, but this ensures
        // that the dialog is not constructed until we want it to be shown the first time,
        // since it will initialise itself on the first load (which causes it to phone
        // home) and we don't want that until the user explicitly asks for it.
        function open() {
            if (item) {
                item.open();
            } else {
                active = true;
            }
        }
        onLoaded: {
            item.open();
        }

        active: false
        asynchronous: true

        sourceComponent: NewStuff.Dialog {
            configFile: "lookandfeel.knsrc"
            viewMode: NewStuff.Page.ViewMode.Preview
            Connections {
                target: newStuffPage.item.engine
                function onEntryEvent(entry, event) {
                    if (event == 2) { // AdoptedEvent
                        kcm.reloadConfig();
                    } else if (event == 1) { // StatusChangedEvent
                        kcm.knsEntryChanged(entry);
                    }
                }
            }
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
