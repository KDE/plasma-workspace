/*
    SPDX-FileCopyrightText: 2022 Dominic Hayes <ferenosdev@outlook.com>
    SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls as QtControls
import org.kde.kirigami as Kirigami
import org.kde.newstuff as NewStuff
import org.kde.kcmutils as KCM
import org.kde.private.kcms.lookandfeel as Private

KCM.GridView {
    id: root

    readonly property bool hasAppearance: kcm.themeContents & Private.LookandFeelManager.AppearanceSettings
    readonly property bool hasLayout: kcm.themeContents & Private.LookandFeelManager.LayoutSettings
    readonly property bool showLayoutInfo: kcm.themeContents & Private.LookandFeelManager.DesktopLayout

    KCM.ConfigModule.buttons: KCM.ConfigModule.Default | KCM.ConfigModule.Help

    function showConfirmation() { //Show the Kirigami Sheet
        if (stackSwitcher.depth !== 1) {
            stackSwitcher.pop();
        }
        resetCheckboxes()
        globalThemeConfirmSheet.open()
        globalThemeConfirmSheet.standardButton(Kirigami.Dialog.Apply).forceActiveFocus()
    }
    function resetCheckboxes() { //This call is used whenever you switch pages (More/Less Options) or trigger the Kirigami Sheet
        kcm.selectedItems = undefined //triggers RESET
    }

    Connections {
        target: kcm
        function onShowConfirmation() {
            root.showConfirmation()
        }
    }

    framedView: false
    view.currentIndex: lookAndFeelRow.index
    view.model: kcm.model

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
                    previewWindow.show(model.fullScreenPreview);
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
                onTriggered: {
                    const component = Qt.createComponent("ConfirmDeletionDialog.qml");
                    component.incubateObject(root, {
                        "pluginId": model.pluginName,
                    });
                }
            }
        ]
        onClicked: {
            kcm.settings.lookAndFeelPackage = model.pluginName;
            showConfirmation();
        }
    }

    Private.ItemModelRow {
        id: lookAndFeelRow
        model: kcm.model
        role: "pluginName"
        value: kcm.settings.lookAndFeelPackage
    }

    Private.LookAndFeelInformation {
        id: selectedLookAndFeelInformation
        model: kcm.model
        packageId: kcm.settings.lookAndFeelPackage
    }

    Kirigami.Dialog {
        id: globalThemeConfirmSheet
        title: i18nc("Confirmation question about applying the Global Theme - %1 is the Global Theme's name",
                     "Apply %1?", selectedLookAndFeelInformation.name)
        standardButtons: Kirigami.Dialog.Apply | Kirigami.Dialog.Cancel
        padding: Kirigami.Units.largeSpacing
        clip: true

        onApplied: {
            kcm.apply();
            globalThemeConfirmSheet.close()
            view.forceActiveFocus() //Prevent further button presses via keyboard
        }
        onRejected: {
            globalThemeConfirmSheet.close()
            view.forceActiveFocus()
        }

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

        footerLeadingComponent: QtControls.Button {
            text: stackSwitcher.depth === 1 ? i18n("Choose what to apply…") : i18n("Show fewer options…")
            icon.name: stackSwitcher.depth === 1 ? "settings-configure" : "go-previous"
            QtControls.DialogButtonBox.buttonRole: QtControls.DialogButtonBox.HelpRole
            enabled: hasAppearance
            onClicked: {
                if (stackSwitcher.depth === 1) {
                    stackSwitcher.push(moreOptions);
                } else {
                    stackSwitcher.pop();
                }
                resetCheckboxes() //Force a refresh to reset button states
            }
        }

        Component.onCompleted: {
            standardButton(Kirigami.Dialog.Apply).enabled = Qt.binding(() => {
                return kcm.selectedContents & (Private.LookandFeelManager.AppearanceSettings
                                             | Private.LookandFeelManager.LayoutSettings
                                             | Private.LookandFeelManager.DesktopLayout);
            });
        }
    }

    PreviewWindow {
        id: previewWindow
    }
}
