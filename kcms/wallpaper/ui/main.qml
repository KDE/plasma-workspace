/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
    SPDX-FileCopyrightText: 2023 Méven Car <meven@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQml

import org.kde.newstuff as NewStuff
import org.kde.kirigami as Kirigami

import org.kde.kcmutils as KCM

import org.kde.plasma.kcm.wallpaper
import org.kde.plasma.configuration 2.0

// Not using AbstractKCM because we're not using any of it features, not even one
Kirigami.ScrollablePage {
    id: appearanceRoot

    title: i18nc("@title:window", "Wallpaper")

    signal configurationChanged

    property alias parentLayout: parentLayout

    implicitWidth: Kirigami.Units.gridUnit * 15
    implicitHeight: Kirigami.Units.gridUnit * 30

    padding: 0

    actions: [
        Kirigami.Action {
            id: allScreensAction
            text: i18nc("@option:check Set the wallpaper for all screens", "Set for all screens")
            visible: kcm.screens.length > 1
            checkable: true
            checked: kcm.allScreens
            onTriggered: kcm.allScreens = checked
            displayComponent: QQC2.Switch {
                text: allScreensAction.text
                checked: allScreensAction.checked
                visible: allScreensAction.visible
                onToggled: allScreensAction.trigger()
            }
        }
    ]

    function onConfigurationChanged() {
        kcm.configuration.keys().forEach(key => {
            const cfgKey = "cfg_" + key;
            if (main.currentItem[cfgKey] !== undefined) {
                kcm.configuration[key] = main.currentItem[cfgKey]
            }
        })
    }

    ColumnLayout {
        height: Math.max(implicitHeight, appearanceRoot.availableHeight)
        width: appearanceRoot.availableWidth

        spacing: 0

        ScreenView {
            visible: !kcm.allScreens && kcm.screens.length > 1

            Layout.fillWidth: true
            Layout.bottomMargin: Kirigami.Units.smallSpacing
            implicitHeight: Kirigami.Units.gridUnit * 10

            outputs: kcm.screens
            selectedScreen: kcm.selectedScreen

            onScreenSelected: (screenName) => { kcm.setSelectedScreen(screenName) }
        }

        Kirigami.FormLayout {
            id: parentLayout // needed for twinFormLayouts to work in wallpaper plugins
            Layout.fillWidth: true

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18nc("@label:listbox", "Wallpaper type:")
                Kirigami.FormData.buddyFor: wallpaperComboBox

                QQC2.ComboBox {
                    id: wallpaperComboBox
                    model: kcm.wallpaperConfigModel
                    textRole: "name"
                    onActivated: {
                        var pluginName = kcm.wallpaperConfigModel.data(kcm.wallpaperConfigModel.index(currentIndex, 0), ConfigModel.PluginNameRole)
                        if (appearanceRoot.currentWallpaper === pluginName) {
                            return;
                        }
                        kcm.currentWallpaper = pluginName
                    }

                    KCM.SettingHighlighter {
                        highlight: kcm.currentWallpaper !== "org.kde.image"
                    }
                }
                NewStuff.Button {
                    configFile: "wallpaperplugin.knsrc"
                    text: i18nc("@action:button", "Get New Plugins…")
                    visibleWhenDisabled: true // don't hide on disabled
                    Layout.preferredHeight: wallpaperComboBox.height
                }
            }
        }

        Item {
            id: emptyConfig
        }

        QQC2.StackView {
            id: main
            implicitHeight: main.empty ? 0 : (currentItem?.implicitHeight ?? 0)

            Layout.fillHeight: true;
            Layout.fillWidth: true;

            Connections {
                target: kcm
                function onCurrentWallpaperChanged () { main.loadSourceFile() }
                function onSelectedScreenChanged () { main.onScreenChanged() }

                function onConfigurationChanged() { main.onWallpaperConfigurationChanged() }
            }

            Connections {
                enabled: main.currentItem?.hasOwnProperty("saveConfig") ?? false
                target: kcm
                function onSettingsSaved() { main.currentItem.saveConfig(); }
            }

            Connections {
                enabled: main.currentItem != null
                target: main.currentItem
                function onConfigurationChanged() { kcm.needsSave = true; }
            }

            Connections {
                enabled: true
                target: kcm.wallpaperConfigModel
                function onWallpaperPluginsChanged() { main.loadSourceFile() }
            }

            function onWallpaperConfigurationChanged() {
                let wallpaperConfig = kcm.configuration
                wallpaperConfig.keys().forEach(key => {
                    const cfgKey = "cfg_" + key;
                    if (cfgKey in main.currentItem) {

                        var changedSignal = main.currentItem[cfgKey + "Changed"]
                        if (changedSignal) {
                            changedSignal.disconnect(appearanceRoot.onConfigurationChanged);
                        }
                        main.currentItem[cfgKey] = wallpaperConfig[key];

                        changedSignal = main.currentItem[cfgKey + "Changed"]
                        if (changedSignal) {
                            changedSignal.connect(appearanceRoot.onConfigurationChanged)
                        }
                    }
                })
            }

            function onScreenChanged() {
                if (!main.currentItem) {
                    main.loadSourceFile();
                    return ;
                }
                main.currentItem.screen = kcm.selectedScreen;
            }

            function loadSourceFile() {
                for (var i = 0; i < kcm.wallpaperConfigModel.count; ++i) {
                    var pluginName = kcm.wallpaperConfigModel.data(kcm.wallpaperConfigModel.index(i, 0), ConfigModel.PluginNameRole)
                    if (kcm.currentWallpaper === pluginName) {
                        wallpaperComboBox.currentIndex = i;
                        break;
                    }
                }

                const wallpaperConfig = kcm.configuration;
                const wallpaperPluginSource = kcm.wallpaperPluginSource
                // BUG 407619: wallpaperConfig can be null before calling `ContainmentItem::loadWallpaper()`
                if (wallpaperConfig && wallpaperPluginSource) {
                    var props = {
                        "configDialog": kcm,
                        "wallpaperConfiguration": wallpaperConfig
                    };

                    // Some third-party wallpaper plugins need the config keys to be set initially.
                    // We should not break them within one Plasma major version, but setting everything
                    // will lead to an error message for every unused property (and some, like KConfigXT
                    // default values, are used by almost no plugin configuration). We load the config
                    // page in a temp variable first, then use that to figure out which ones we need to
                    // set initially.
                    // TODO Plasma 7: consider whether we can drop this workaround
                    const temp = Qt.createComponent(Qt.resolvedUrl(wallpaperPluginSource)).createObject(appearanceRoot, props)
                    wallpaperConfig.keys().forEach(key => {
                        const cfgKey = "cfg_" + key;
                        if (cfgKey in temp) {
                            props[cfgKey] = wallpaperConfig[key]
                        }
                    })
                    if ("screen" in temp) {
                        props["screen"] = kcm.selectedScreen
                    }
                    temp.destroy()

                    var newItem = replace(Qt.resolvedUrl(wallpaperPluginSource), props)

                    wallpaperConfig.keys().forEach(key => {
                        const cfgKey = "cfg_" + key;
                        if (cfgKey in newItem) {
                            let changedSignal = main.currentItem[cfgKey + "Changed"]
                            if (changedSignal) {
                                changedSignal.connect(appearanceRoot.onConfigurationChanged)
                            }
                        }
                    });

                    const configurationChangedSignal = newItem.configurationChanged
                    if (configurationChangedSignal) {
                        configurationChangedSignal.connect(appearanceRoot.onConfigurationChanged)
                    }
                } else {
                    replace(emptyConfig)
                }
            }
        }
    }

}
