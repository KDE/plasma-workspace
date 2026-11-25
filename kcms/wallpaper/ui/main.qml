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
import org.kde.plasma.configuration

// Not using AbstractKCM because we're not using any of it features, not even one
Kirigami.ScrollablePage {
    id: appearanceRoot

    title: i18nc("@title:window", "Wallpaper")

    signal configurationChanged

    property alias parentLayout: parentLayout

    readonly property bool contentReady: main.currentItem !== null

    implicitWidth: Kirigami.Units.gridUnit * 15
    implicitHeight: Kirigami.Units.gridUnit * 30

    padding: 0

    QQC2.ActionGroup {
        id: wallpaperPluginGroup
        onCheckedActionChanged: {
            kcm.currentWallpaper = checkedAction.pluginName
            let idx = kcm.wallpaperConfigModel.index(checkedAction.index, 0)
            main.sourceFile = idx.data(ConfigModel.SourceRole)
        }
    }
    Connections {
        target: kcm.wallpaperConfigModel
        function onWallpaperPluginsChanged() {
            wallPaperTypeAction.reset()
        }
    }

    component ModelAction: Kirigami.Action {
        id: modelAction
        property alias model: internal.model
        property alias delegate: internal.delegate
        readonly property Instantiator __instantiator: Instantiator {
            id: internal
            onObjectAdded: (index, action) => modelAction.children.push(action)
            onObjectRemoved: (index, action) => modelAction.children.splice(index, 1)
        }
        function reset() {
            internal.active = false
            children = []
            internal.active = true
        }
    }

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
        },
        ModelAction {
            id: wallPaperTypeAction
            text: i18nc("title:group, listbox-like button menu, %1 is wallpaper plugin name",
                        "Type: %1", wallpaperPluginGroup.checkedAction?.name ?? "")
            model: kcm.wallpaperConfigModel
            delegate: Kirigami.Action {
                id: actionDelegate
                required property string name
                required property string pluginName
                required property int index

                QQC2.ActionGroup.group: wallpaperPluginGroup
                text: name
                checkable: true
                checked: pluginName === kcm.currentWallpaper
            }
        },
        NewStuff.Action {
            configFile: "wallpaperplugin.knsrc"
            text: i18ndc("plasma_shell_org.kde.plasma.desktop", "@action:button", "Get New Plugins…")
        }
    ]

    KCM.SettingHighlighter {
        id: toolBarWallpaperTypeHighlighter
        highlight: kcm.currentWallpaper !== "org.kde.image"
    }

    onGlobalToolBarItemChanged: {
        // HACK: I don't think we can do SettingHighlighter cleanly with the global toolbar, and can't easily
        // set a custom display component as the default one is private and I don't want to copy it here.
        // I hope Union semantic hints will make this cleaner.
        let typeActionButtons = globalToolBarItem?.contentItem[1].children[4].contentItem.children.filter(
            (entry) => entry.action == wallPaperTypeAction)

        if (! typeActionButtons || typeActionButtons?.length < 1) {
            return
        }

        toolBarWallpaperTypeHighlighter.target = typeActionButtons.pop()
        // needs to be non-flat when necessary - flat doesn't show a highlight
        toolBarWallpaperTypeHighlighter.target.flat = Qt.binding(() => !toolBarWallpaperTypeHighlighter.highlight)
    }

    function onConfigurationChanged() {
        kcm.configuration.keys().forEach(key => {
            const cfgKey = "cfg_" + key;
            if (main.currentItem[cfgKey] !== undefined) {
                kcm.configuration[key] = main.currentItem[cfgKey]
            }
        })
    }

    ColumnLayout {

        // We want to hide the content until the plugin is loaded, otherwise layout will flicker for a couple of frames
        opacity: appearanceRoot.contentReady ? 1 : 0

        height: Math.max(implicitHeight, appearanceRoot.availableHeight)
        width: appearanceRoot.availableWidth

        spacing: 0

        ScreenView {
            visible: !kcm.allScreens && kcm.screens.length > 1

            Layout.fillWidth: true
            implicitHeight: Kirigami.Units.gridUnit * 10

            outputs: kcm.screens
            selectedScreen: kcm.selectedScreen

            onScreenSelected: (screenName) => { kcm.setSelectedScreen(screenName) }
        }

        // only here for backward compatibility with FormLayout wallpaper plugins
        // TODO Plasma 7: remove and document in porting guide
        Kirigami.FormLayout {
            id: parentLayout // needed for twinFormLayouts to work in wallpaper plugins
        }

        Item {
            id: emptyConfig
        }

        QQC2.StackView {
            id: main
            implicitHeight: main.empty ? 0 : (currentItem?.implicitHeight ?? 0)

            Layout.fillHeight: true;
            Layout.fillWidth: true;

            property string sourceFile: "tbd"

            onSourceFileChanged: loadSourceFile()

            Connections {
                target: kcm
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
                ignoreUnknownSignals: true
                function onConfigurationChanged() { kcm.needsSave = true; }
            }

            function onWallpaperConfigurationChanged() {
                let wallpaperConfig = kcm.configuration
                if (!main.currentItem) {
                    return
                }
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
                    return ;
                }
                main.currentItem.screen = kcm.selectedScreen;
            }

            function loadSourceFile() {
                const wallpaperConfig = kcm.configuration;
                const wallpaperPluginSource = main.sourceFile
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
                    // Hidden because destroy() only takes effect at the end of the event loop,
                    // so this second copy of the config page would be painted meanwhile.
                    const temp = Qt.createComponent(Qt.resolvedUrl(wallpaperPluginSource))
                                   .createObject(appearanceRoot, Object.assign({"visible": false}, props))
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
