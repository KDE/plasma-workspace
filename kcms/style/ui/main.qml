/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.6
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.5 as Kirigami
import org.kde.kcmutils as KCM
import org.kde.private.kcms.style 1.0 as Private

KCM.GridViewKCM {
    id: root

    view.model: kcm.model
    view.currentIndex: kcm.model.selectedStyleIndex

    KCM.SettingStateBinding {
        configObject: kcm.styleSettings
        settingName: "widgetStyle"
    }

    function openGtkStyleSettings() {
        kcm.push("GtkStylePage.qml");
    }

    Component.onCompleted: {
        // The widget thumbnails are a bit more elaborate and need more room, especially when translated
        view.implicitCellWidth = Kirigami.Units.gridUnit * 21;
        view.implicitCellHeight = Kirigami.Units.gridUnit * 15;
    }

    // putting the InlineMessage as header item causes it to show up initially despite visible false
    header: ColumnLayout {
        Kirigami.InlineMessage {
            id: infoLabel
            Layout.fillWidth: true

            showCloseButton: true
            visible: false

            Connections {
                target: kcm
                function onShowErrorMessage(message) {
                    infoLabel.type = Kirigami.MessageType.Error;
                    infoLabel.text = message;
                    infoLabel.visible = true;
                }
            }
        }
    }

    actions: [
        Kirigami.Action {
            id: effectSettingsButton
            text: i18n("Configure Icons and Toolbars")
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

        thumbnailAvailable: thumbnailItem.valid
        thumbnail: Private.PreviewItem {
            id: thumbnailItem
            anchors.fill: parent

            smooth: false
            styleName: model.styleName

            Connections {
                target: kcm
                function onStyleReconfigured(message) {
                    if (styleName === model.styleName) {
                        thumbnailItem.reload();
                    }
                }
            }
        }

        actions: [
            Kirigami.Action {
                icon.name: "document-edit"
                tooltip: i18n("Configure Style…")
                enabled: model.configurable
                onTriggered: kcm.configure(model.display, model.styleName, delegate)
            }
        ]
        onClicked: {
            kcm.model.selectedStyle = model.styleName;
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
}
