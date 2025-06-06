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
import org.kde.private.kcms.lookandfeel as Private
import org.kde.kcmutils as KCM
import org.kde.kitemmodels as KItemModels

KCM.GridViewKCM {
    id: selector

    required property int variant

    title: {
        if (variant === Private.LookAndFeel.Variant.Light) {
            return i18nc("@title:window", "Choose Light Global Theme");
        } else {
            return i18nc("@title:window", "Choose Dark Global Theme");
        }
    }
    view.currentIndex: selectedLookAndFeelRow.index
    view.model: lookAndFeelPackages

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
                    component.incubateObject(selector, {
                        "pluginId": model.pluginName,
                    });
                }
            }
        ]
        onClicked: {
            if (selector.variant === Private.LookAndFeel.Variant.Light) {
                kcm.settings.defaultLightLookAndFeel = model.pluginName;
            } else if (selector.variant === Private.LookAndFeel.Variant.Dark) {
                kcm.settings.defaultDarkLookAndFeel = model.pluginName;
            }
            kcm.pop();
        }
    }

    KItemModels.KSortFilterProxyModel {
        id: lookAndFeelPackages
        sourceModel: kcm.model
        filterRoleName: "variant"
        filterRowCallback: (sourceRow, sourceParent) => {
            let value = sourceModel.data(sourceModel.index(sourceRow, 0, sourceParent), filterRole);
            if (value === Private.LookAndFeel.Variant.Unknown) {
                return true;
            } else {
                return value === variant;
            }
        }
    }

    Private.ItemModelRow {
        id: selectedLookAndFeelRow
        model: lookAndFeelPackages
        role: "pluginName"
        value: variant === Private.LookAndFeel.Variant.Light ? kcm.settings.defaultLightLookAndFeel : kcm.settings.defaultDarkLookAndFeel
    }

    PreviewWindow {
        id: previewWindow
    }
}
