/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2016 David Rosca <nowrep@gmail.com>
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 Kevin Ottens <kevin.ottens@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/
import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.0
import QtQuick.Controls 2.3 as QtControls
import QtQml 2.15
import org.kde.kirigami 2.8 as Kirigami
import org.kde.newstuff 1.81 as NewStuff
import org.kde.kcm 1.3 as KCM
import org.kde.private.kcms.desktoptheme 1.0 as Private

KCM.GridViewKCM {
    id: root
    KCM.ConfigModule.quickHelp: i18n("This module lets you choose the Plasma style.")

    view.model: kcm.filteredModel
    view.currentIndex: kcm.filteredModel.selectedThemeIndex

    Binding {
        target: kcm.filteredModel
        property: "query"
        value: searchField.text
        restoreMode: Binding.RestoreBinding
    }

    Binding {
        target: kcm.filteredModel
        property: "filter"
        value: filterCombo.model[filterCombo.currentIndex].filter
        restoreMode: Binding.RestoreBinding
    }

    KCM.SettingStateBinding {
        configObject: kcm.desktopThemeSettings
        settingName: "name"
        extraEnabledConditions: !kcm.downloadingFile
    }

    DropArea {
        anchors.fill: parent
        onEntered: {
            if (!drag.hasUrls) {
                drag.accepted = false;
            }
        }
        onDropped: kcm.installThemeFromFile(drop.urls[0])
    }
    header: RowLayout {
        Layout.fillWidth: true

        Kirigami.SearchField {
            id: searchField
            Layout.fillWidth: true
        }
        QtControls.ComboBox {
            id: filterCombo
            textRole: "text"
            model: [{
                    "text": i18n("All Themes"),
                    "filter": Private.FilterProxyModel.AllThemes
                }, {
                    "text": i18n("Light Themes"),
                    "filter": Private.FilterProxyModel.LightThemes
                }, {
                    "text": i18n("Dark Themes"),
                    "filter": Private.FilterProxyModel.DarkThemes
                }, {
                    "text": i18n("Color scheme compatible"),
                    "filter": Private.FilterProxyModel.ThemesFollowingColors
                }]

            // HACK QQC2 doesn't support icons, so we just tamper with the desktop style ComboBox's background
            // and inject a nice little filter icon.
            Component.onCompleted: {
                if (!background || !background.hasOwnProperty("properties")) {
                    // not a KQuickStyleItem
                    return;
                }
                var props = background.properties || {};
                background.properties = Qt.binding(function () {
                        var newProps = props;
                        newProps.currentIcon = "view-filter";
                        newProps.iconColor = Kirigami.Theme.textColor;
                        return newProps;
                    });
            }
        }
    }

    view.delegate: KCM.GridDelegate {
        id: delegate

        text: model.display
        subtitle: model.colorType == Private.ThemesModel.FollowsColorTheme && view.model.filter != Private.FilterProxyModel.ThemesFollowingColors ? i18n("Follows color scheme") : ""
        toolTip: model.description || model.display

        opacity: model.pendingDeletion ? 0.3 : 1
        Behavior on opacity  {
            NumberAnimation {
                duration: Kirigami.Units.longDuration
            }
        }

        thumbnailAvailable: true
        thumbnail: ThemePreview {
            id: preview
            anchors.fill: parent
            themeName: model.pluginName
        }

        actions: [
            Kirigami.Action {
                iconName: "document-edit"
                tooltip: i18n("Edit Theme…")
                enabled: !model.pendingDeletion
                visible: kcm.canEditThemes
                onTriggered: kcm.editTheme(model.pluginName)
            },
            Kirigami.Action {
                iconName: "edit-delete"
                tooltip: i18n("Remove Theme")
                enabled: model.isLocal
                visible: !model.pendingDeletion
                onTriggered: model.pendingDeletion = true
            },
            Kirigami.Action {
                iconName: "edit-undo"
                tooltip: i18n("Restore Theme")
                visible: model.pendingDeletion
                onTriggered: model.pendingDeletion = false
            }
        ]

        onClicked: {
            kcm.desktopThemeSettings.name = model.pluginName;
            view.forceActiveFocus();
        }
        onDoubleClicked: {
            kcm.save();
        }
    }

    footer: ColumnLayout {
        Kirigami.InlineMessage {
            id: infoLabel
            Layout.fillWidth: true

            showCloseButton: true

            Connections {
                target: kcm
                function onShowSuccessMessage(message) {
                    infoLabel.type = Kirigami.MessageType.Positive;
                    infoLabel.text = message;
                    infoLabel.visible = true;
                }
                function onShowErrorMessage(message) {
                    infoLabel.type = Kirigami.MessageType.Error;
                    infoLabel.text = message;
                    infoLabel.visible = true;
                }
            }
        }

        Kirigami.ActionToolBar {
            flat: false
            alignment: Qt.AlignRight
            actions: [
                Kirigami.Action {
                    text: i18n("Install from File…")
                    icon.name: "document-import"
                    onTriggered: fileDialogLoader.active = true
                },
                NewStuff.Action {
                    text: i18n("Get New Plasma Styles…")
                    configFile: "plasma-themes.knsrc"
                    onEntryEvent: function (entry, event) {
                        kcm.load();
                    }
                }
            ]
        }
    }

    Loader {
        id: fileDialogLoader
        active: false
        sourceComponent: FileDialog {
            title: i18n("Open Theme")
            folder: shortcuts.home
            nameFilters: [i18n("Theme Files (*.zip *.tar.gz *.tar.bz2)")]
            Component.onCompleted: open()
            onAccepted: {
                kcm.installThemeFromFile(fileUrls[0]);
                fileDialogLoader.active = false;
            }
            onRejected: {
                fileDialogLoader.active = false;
            }
        }
    }
}
