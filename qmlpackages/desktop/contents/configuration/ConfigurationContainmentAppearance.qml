/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
 */

import QtQuick 2.0
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.configuration 2.0
import QtQuick.Controls 1.0 as QtControls
import QtQuick.Layouts 1.0

ColumnLayout {
    id: root

    spacing: _m
    PlasmaExtras.Title {
        text: "Plugins"
    }

//BEGIN functions
    function saveConfig() {
        for (var key in configDialog.wallpaperConfiguration) {
            if (main.currentItem["cfg_"+key] !== undefined) {
                configDialog.wallpaperConfiguration[key] = main.currentItem["cfg_"+key]
            }
        }
        configDialog.applyWallpaper()
    }

    function restoreConfig() {
        for (var key in configDialog.wallpaperConfiguration) {
            if (main.currentItem["cfg_"+key] !== undefined) {
                main.currentItem["cfg_"+key] = configDialog.wallpaperConfiguration[key]
            }
        }
    }
//END functions

    ListView {
        id: categoriesView
        anchors {
            left: parent.left
            right: parent.right
        }
        height: 100
        orientation: ListView.Horizontal 

        model: configDialog.wallpaperConfigModel
        delegate: ConfigCategoryDelegate {
            id: delegate
            current: categoriesView.currentIndex == index
            anchors {
                top: parent.top
                bottom: parent.bottom
                left: undefined
                right: undefined
            }
            width: 64
            onClicked: {
                configDialog.currentWallpaper = model.pluginName
                if (categoriesView.currentIndex == index) {
                    return
                } else {
                    categoriesView.currentIndex = index;
                    main.sourceFile = model.source
                    root.restoreConfig()
                }
            }
            onCurrentChanged: {
                categoriesView.currentIndex = index
                if (current) {
                    categoriesView.currentIndex = index
                }
            }
            Component.onCompleted: {
                if (configDialog.currentWallpaper == model.pluginName) {
                    loadWallpaperTimer.pendingCurrent = model
                    loadWallpaperTimer.restart()
                }
            }
        }
        highlight: Rectangle {
            color: theme.highlightColor
        }
        Timer {
            id: loadWallpaperTimer
            interval: 100
            property variant pendingCurrent
            onTriggered: {
                if (pendingCurrent) {
                    categoriesView.currentIndex = pendingCurrent.index
                    main.sourceFile = pendingCurrent.source
                    root.restoreConfig()
                }
            }
        }
    }

    Row {
        spacing: 10
        QtControls.Label {
            anchors.verticalCenter: pluginCombobox.verticalCenter
            text: "Wallpaper plugin:"
        }
        QtControls.ComboBox {
            id: pluginCombobox
            model: configDialog.wallpaperConfigModel
            textRole: "name"
        }
    }

    QtControls.StackView {
        id: main
        Layout.fillHeight: true;
        anchors {
            left: categoriesView.left;
            right: parent.right;
        }
        property string sourceFile
        onSourceFileChanged: {
            if (sourceFile != "") {
                replace(Qt.resolvedUrl(sourceFile))
            }
        }
    }
}
