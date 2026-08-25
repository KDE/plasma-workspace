/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs

import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM
import org.kde.private.kcms.style as Private

KCM.GridViewKCM {
    id: root

    view.model: kcm.model
    view.currentIndex: {
        let widgetStyle = kcm.styleSettings.widgetStyle
        let unionStyle = kcm.styleSettings.unionStyle

        for (let i = 0; i < view.count; ++i) {
            let styleId = kcm.model.data(kcm.model.index(i, 0), Private.StylesModel.StyleIdRole)
            let isUnionStyle = kcm.model.data(kcm.model.index(i, 0), Private.StylesModel.IsUnionStyle)

            if (widgetStyle === "Union") {
                if (isUnionStyle && styleId == unionStyle) {
                    return i
                }
            } else if (styleId == widgetStyle) {
                return i
            }
        }
        return -1
    }
    // The widget thumbnails are a bit more elaborate and need more room, especially when translated
    view.implicitCellWidth: Kirigami.Units.gridUnit * 21;
    view.implicitCellHeight: Kirigami.Units.gridUnit * 15;

    KCM.SettingStateBinding {
        configObject: kcm.styleSettings
        settingName: "widgetStyle"
    }

    KCM.SettingStateBinding {
        configObject: kcm.styleSettings
        settingName: "unionStyle"
    }

    function openGtkStyleSettings() {
        kcm.push("GtkStylePage.qml");
    }

    headerPaddingEnabled: false // Let the InlineMessage touch the edges
    header: Kirigami.InlineMessage {
        id: infoLabel

        position: Kirigami.InlineMessage.Position.Header
        showCloseButton: true
        visible: false

        function showMessage(messageType, message)
        {
            type = messageType
            text = message
            visible = true
        }

        Connections {
            target: kcm
            function onShowErrorMessage(message) {
                infoLabel.showMessage(Kirigami.MessageType.Error, message)
            }

            function onShowInfoMessage(message) {
                infoLabel.showMessage(Kirigami.MessageType.Information, message)
            }
        }
    }

    actions: [
        Kirigami.Action {
            icon.name: "document-import-symbolic"
            text: i18nc("@action:button", "Install From File…")
            onTriggered: installDialog.open()
        },
        Kirigami.Action {
            id: effectSettingsButton
            text: i18n("Configure Icons and Toolbars…")
            icon.name: "configure-toolbars" // proper icon?
            checkable: true
            checked: effectSettingsPopupLoader.popupOpen
            onTriggered: {
                if (effectSettingsPopupLoader.popupOpen) {
                    effectSettingsPopupLoader.item.close()
                    // We don't set the Loader to inactive here since that would
                    // happen before the popup has closed; instead we use a
                    // Connections object in the Loader itself to handle it
                } else {
                    effectSettingsPopupLoader.active = true;
                    effectSettingsPopupLoader.item.open();
                }
            }
        },
        Kirigami.Action {
            visible: kcm.gtkConfigKdedModuleLoaded
            text: i18n("Configure GNOME/GTK Application Style…")
            icon.name: "configure"
            onTriggered: root.openGtkStyleSettings()
        }
    ]

    view.delegate: KCM.GridDelegate {
        id: delegate

        text: model.display
        toolTip: model.description

        thumbnailAvailable: thumbnailLoader.item?.valid ?? false
        thumbnail: Loader {
            id: thumbnailLoader

            anchors.fill: parent
            clip: true
            opacity: kcm.stylesToUninstall.includes(model.styleId) ? 0.3 : 1.0

            Component {
                id: widgetsThumbnail

                Private.PreviewItem {
                    id: thumbnailItem

                    smooth: false
                    styleName: model.styleName

                    Connections {
                        target: kcm
                        function onStyleReconfigured(message) {
                            if (model.styleName === model.styleName) {
                                thumbnailItem.reload();
                            }
                        }
                    }
                }
            }

            Component {
                id: unionThumbnail

                UnionThumbnailItem {
                    property bool valid: true

                    styleId: model.styleId

                    onSelected: {
                        kcm.styleSettings.widgetStyle = "Union"
                        kcm.styleSettings.unionStyle = model.styleId
                    }
                }
            }

            sourceComponent: model.isUnionStyle ? unionThumbnail : widgetsThumbnail
        }

        actions: [
            Kirigami.Action {
                icon.name: "info-symbolic"
                visible: model.isUnionStyle
                tooltip: i18nc("@info:tooltip", "This style is using the Union style engine which is still under development. Some things may look or behave incorrectly when using this style.")
            },
            Kirigami.Action {
                icon.name: "document-edit"
                tooltip: i18n("Configure Style…")
                enabled: model.configurable
                onTriggered: kcm.configure(model.display, model.styleName, delegate)
            },
            Kirigami.Action {
                icon.name:  "delete-symbolic"
                tooltip: {
                    if (model.canUninstall) {
                        return i18nc("@action:button", "Uninstall…")
                    } else if (model.isUnionStyle) {
                        return i18nc("@action:button", "Cannot uninstall system-installed styles.")
                    } else {
                        return i18nc("@action:button", "Can only uninstall Union styles.")
                    }
                }
                enabled: model.canUninstall
                visible: !kcm.stylesToUninstall.includes(model.styleId)
                onTriggered: {
                    let uninstall = kcm.stylesToUninstall
                    uninstall.push(model.styleId)
                    kcm.stylesToUninstall = uninstall
                }
            },
            Kirigami.Action {
                icon.name: "edit-undo"
                tooltip: i18n("Do not uninstall this style.")
                visible: kcm.stylesToUninstall.includes(model.styleId)
                onTriggered: {
                    let uninstall = kcm.stylesToUninstall
                    uninstall.splice(uninstall.indexOf(model.styleId), 1)
                    kcm.stylesToUninstall = uninstall
                }
            }
        ]
        onClicked: {
            if (model.isUnionStyle) {
                kcm.styleSettings.widgetStyle = "Union"
                kcm.styleSettings.unionStyle = model.styleId
            } else {
                kcm.styleSettings.widgetStyle = model.styleId
                kcm.styleSettings.unionStyle = kcm.styleSettings.defaultUnionStyleValue
            }
            view.forceActiveFocus();
        }
        onDoubleClicked: {
            kcm.save();
        }
    }

    Loader {
        id: effectSettingsPopupLoader

        readonly property bool popupOpen: Boolean(effectSettingsPopupLoader.item?.opened)

        active: false
        sourceComponent: EffectSettingsPopup {
            parent: root
        }

        Connections {
            enabled: effectSettingsPopupLoader.active
            target: effectSettingsPopupLoader.item
            function onClosed() {
                effectSettingsPopupLoader.active = false;
            }
        }
    }

    FileDialog {
        id: installDialog

        title: i18nc("@title:dialog", "Choose Union Style to Install")
        acceptLabel: i18nc("@action:button", "Install")
        options: FileDialog.ReadOnly

        nameFilters: ["Union Styles (*.unionstyle)"]

        onAccepted: kcm.installUnionStyle(selectedFile)
    }
}
