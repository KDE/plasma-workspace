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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.plasma.configuration 0.1

Column {
    id: root
    PlasmaComponents.Label {
        text: "Plugins"
    }


//BEGIN functions
    function saveConfig() {
        for (var key in configDialog.wallpaperConfiguration) {
            if (main.currentPage["cfg_"+key] !== undefined) {
                configDialog.wallpaperConfiguration[key] = main.currentPage["cfg_"+key]
            }
        }
    }

    function restoreConfig() {
        for (var key in configDialog.wallpaperConfiguration) {
            if (main.currentPage["cfg_"+key] !== undefined) {
                main.currentPage["cfg_"+key] = configDialog.wallpaperConfiguration[key]
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
            anchors {
                top: parent.top
                bottom: parent.bottom
                left: undefined
                right: undefined
            }
            width: 64
            onClicked: {
                if (delegate.current) {
                    return
                } else {
                    configDialog.currentWallpaper = model.pluginName
                    main.sourceFile = model.source
                    root.restoreConfig()
                }
            }
            onCurrentChanged: {
                if (current) {
                    categoriesView.currentIndex = index
                }
            }
        }
        highlight: Rectangle {
            color: theme.highlightColor
        }
    }
    PlasmaComponents.PageStack {
        id: main
        anchors.horizontalCenter: parent.horizontalCenter
        width: implicitWidth
        height: implicitHeight
        property string sourceFile
        onSourceFileChanged: {
            replace(Qt.resolvedUrl(sourceFile))
            root.width = mainColumn.implicitWidth
            root.height = mainColumn.implicitHeight
        }
    }
}
