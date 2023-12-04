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

KCM.SimpleKCM {
    id: appearanceRoot
    
    signal configurationChanged
    
    property alias parentLayout: parentLayout
    
    implicitWidth: Kirigami.Units.gridUnit * 15
    implicitHeight: Kirigami.Units.gridUnit * 30
    
    function onConfigurationChanged() {
        for (var key in kcm.configuration) {
            const cfgKey = "cfg_" + key;
            if (main.currentItem[cfgKey] !== undefined) {
                kcm.configuration[key] = main.currentItem[cfgKey]
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        
        ScreenView {
            visible: kcm.screens.length > 1
            
            Layout.fillWidth: true;
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
                Kirigami.FormData.label: i18nd("plasma_shell_org.kde.plasma.desktop", "Wallpaper type:")

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
                    text: i18nd("plasma_shell_org.kde.plasma.desktop", "Get New Plugins…")
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

            Layout.fillHeight: true;
            Layout.fillWidth: true;
                     
            Connections {
                target: kcm
                function onCurrentWallpaperChanged () { main.loadSourceFile() }
                function onSelectedScreenChanged () { main.onScreenChanged() }
                
                function onConfigurationChanged() { main.onWallpaperConfigurationChanged() }

                function onSettingsSaved() { main.currentItem.saveConfig(); }
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
                        "screen": kcm.selectedScreen,
                        "wallpaperConfiguration": wallpaperConfig
                    };
                                            
                    wallpaperConfig.keys().forEach(key => {
                        // Preview is not part of the config, only of the WallpaperObject
                        if (!key.startsWith("Preview")) {
                            props["cfg_" + key] = wallpaperConfig[key];
                        }
                    });
                    
                    var newItem = replace(Qt.resolvedUrl(wallpaperPluginSource), props)
                        
                    wallpaperConfig.keys().forEach(key => {
                        const cfgKey = "cfg_" + key;
                        if (cfgKey in main.currentItem) {
                            var changedSignal = main.currentItem[cfgKey + "Changed"]
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
