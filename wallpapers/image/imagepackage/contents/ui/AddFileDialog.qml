/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtCore
import QtQuick
import QtQuick.Dialogs as QtDialogs

Loader {
    id: dialogLoader

    asynchronous: true
    sourceComponent: configDialog.currentWallpaper === "org.kde.image" ? addFileDialog : addFolderDialog

    readonly property url defaultFolder: {
        let defaultPaths = StandardPaths.standardLocations(StandardPaths.PicturesLocation);
        if (defaultPaths.length === 0) {
            // HomeLocation is guaranteed not to be empty.
            defaultPaths = StandardPaths.standardLocations(StandardPaths.HomeLocation);
        }
        return defaultPaths[0];
    }

    Connections {
        target: dialogLoader.item
        function onAccepted() {
            let added = false;
            if (dialogLoader.item instanceof QtDialogs.FileDialog) {
                dialogLoader.item.selectedFiles.forEach(url => {
                    added |= imageWallpaper.addUsersWallpaper(url).length > 0;
                });
            } else if (dialogLoader.item instanceof QtDialogs.FolderDialog) {
                added = imageWallpaper.addSlidePath(dialogLoader.item.selectedFolder);
            }
            if (added) {
                root.wallpaperBrowseCompleted();
                root.configurationChanged();
            }
            dialogLoader.destroy();
        }
        function onRejected() {
            dialogLoader.destroy();
        }
    }

    Component {
        id: addFileDialog

        QtDialogs.FileDialog {
            id: fileDialog
            visible: dialogLoader.status === Loader.Ready
            currentFolder: dialogLoader.defaultFolder
            nameFilters: imageWallpaper.nameFilters()
            fileMode: QtDialogs.FileDialog.OpenFiles
            options: QtDialogs.FileDialog.ReadOnly
            title: i18nc("@title:window", "Open Image")
        }
    }

    Component {
        id: addFolderDialog

        QtDialogs.FolderDialog {
            id: folderDialog
            visible: dialogLoader.status === Loader.Ready
            currentFolder: dialogLoader.defaultFolder
            options: QtDialogs.FolderDialog.ReadOnly
            title: i18nc("@title:window", "Directory with the wallpaper to show slides from")
        }
    }
}
