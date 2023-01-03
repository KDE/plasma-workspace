/*
    SPDX-FileCopyrightText: 2018 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2022 Dominic Hayes <ferenosdev@outlook.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

import QtQuick 2.6
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import QtQuick.Controls 2.3 as QtControls
import org.kde.kirigami 2.8 as Kirigami
import org.kde.newstuff 1.91 as NewStuff
import org.kde.kconfig 1.0 // for KAuthorized
import org.kde.kcm 1.3 as KCM
import org.kde.private.kcms.lookandfeel 1.0 as Private

KCM.GridViewKCM {
    id: root

    KCM.ConfigModule.quickHelp: i18n("This module lets you choose the global look and feel.")

    readonly property bool hasAppearance: currentThemeRole(Private.KCMLookandFeel.HasGlobalThemeRole)
    readonly property bool hasLayout: currentThemeRole(Private.KCMLookandFeel.HasLayoutSettingsRole)
                                        || currentThemeRole(Private.KCMLookandFeel.HasDesktopLayoutRole)
    readonly property bool showLayoutInfo: currentThemeRole(Private.KCMLookandFeel.HasDesktopLayoutRole)

    function currentThemeRole(role) {
        return view.model.data(view.model.index(view.currentIndex, 0), role)
    }
    function showConfirmation() { //Show the Kirigami Sheet
        if (stackSwitcher.depth !== 1) {
            stackSwitcher.pop();
        }
        resetCheckboxes()
        globalThemeConfirmSheet.open()
        proceedButton.forceActiveFocus()
    }
    function resetCheckboxes() { //This call is used whenever you switch pages (More/Less Options) or trigger the Kirigami Sheet
        kcm.appearanceToApply = undefined //triggers RESET
        kcm.layoutToApply = undefined
    }
    Connections {
        target: kcm
        function onShowConfirmation() {
            root.showConfirmation()
        }
    }

    view.model: kcm.lookAndFeelModel
    view.currentIndex: kcm.pluginIndex(kcm.lookAndFeelSettings.lookAndFeelPackage)

    KCM.SettingStateBinding {
        configObject: kcm.lookAndFeelSettings
        settingName: "lookAndFeelPackage"
    }

    view.delegate: KCM.GridDelegate {
        id: delegate

        text: model.display
        subtitle: model.hasDesktopLayout ? i18n("Contains Desktop layout") : ""
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
                    previewWindow.url = model.fullScreenPreview
                    previewWindow.showFullScreen()
                }
            }
        ]
        onClicked: {
            kcm.lookAndFeelSettings.lookAndFeelPackage = model.pluginName
            root.showConfirmation()
        }
    }

    Kirigami.OverlaySheet {
        id: globalThemeConfirmSheet
        title: i18nc("Confirmation question about applying the Global Theme - %1 is the Global Theme's name",
                     "Apply %1?", currentThemeRole(Qt.Display))
        QtControls.StackView {
            id: stackSwitcher
            initialItem: simpleOptions
            implicitHeight: currentItem.implicitHeight
            implicitWidth: Kirigami.Units.gridUnit * 30
            Component {
                id: simpleOptions
                SimpleOptions {}
            }
            Component {
                id: moreOptions
                MoreOptions {}
            }
        }
        footer: ColumnLayout {
            RowLayout {
                QtControls.Button {
                    text: stackSwitcher.depth === 1 ? i18n("Choose what to apply…") : i18n("Show fewer options…")
                    icon.name: stackSwitcher.depth === 1 ? "settings-configure" : "go-previous"
                    enabled: currentThemeRole(Private.KCMLookandFeel.HasGlobalThemeRole)
                    onClicked: {
                        if (stackSwitcher.depth === 1) {
                            stackSwitcher.push(moreOptions);
                        } else {
                            stackSwitcher.pop();
                        }
                        resetCheckboxes() //Force a refresh to reset button states
                    }
                }
                Rectangle {
                    Layout.fillWidth: true
                }
                QtControls.Button {
                    id: proceedButton
                    text: i18n("Apply")
                    icon.name: "dialog-ok-apply"
                    onClicked: {
                        kcm.save()
                        globalThemeConfirmSheet.close()
                        view.forceActiveFocus() //Prevent further button presses via keyboard
                    }
                    enabled: kcm.appearanceToApply & Private.LookandFeelManager.AppearanceSettings ||
                        kcm.layoutToApply & Private.LookandFeelManager.LayoutSettings ||
                        kcm.layoutToApply & Private.LookandFeelManager.DesktopLayout
                }
                QtControls.Button {
                    text: i18n("Cancel")
                    icon.name: "dialog-cancel"
                    onClicked: {
                        globalThemeConfirmSheet.close()
                        view.forceActiveFocus()
                    }
                }
            }
        }
    }

    footer: Kirigami.ActionToolBar {
        flat: false
        alignment: Qt.AlignRight
        actions: [
            NewStuff.Action {
                configFile: "lookandfeel.knsrc"
                text: i18n("Get New Global Themes…")
                onEntryEvent: function (entry, event) {
                    if (event == NewStuff.Entry.StatusChangedEvent) {
                        kcm.knsEntryChanged(entry);
                    } else if (event == NewStuff.Entry.AdoptedEvent) {
                        kcm.reloadConfig();
                    }
                }
            }
        ]
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
