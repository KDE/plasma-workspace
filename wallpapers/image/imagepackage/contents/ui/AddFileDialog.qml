/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
pragma ComponentBehavior: Bound


import QtQuick
import QtQuick.Dialogs as QtDialogs

Loader {
    id: dialogLoader

    asynchronous: true
    sourceComponent: configDialog.currentWallpaper === "org.kde.image" ? addFileDialog : addFolderDialog

    Connections {
        target: dialogLoader.item
        function onAccepted() {
            let added = false;
            if (dialogLoader.item instanceof QtDialogs.FileDialog) {
                const fileDialog = dialogLoader.item as QtDialogs.FileDialog;
                const folder = fileDialog.currentFolder;
                fileDialog.selectedFiles.forEach(url => {
                    added |= imageWallpaper.addUsersWallpaper(url).length > 0;
                });
                imageWallpaper.lastFolder = folder;
            } else if (dialogLoader.item instanceof QtDialogs.FolderDialog) {
                const folderDialog = dialogLoader.item as QtDialogs.FolderDialog;
                const folder = folderDialog.currentFolder;
                added = imageWallpaper.addSlidePath(folderDialog.selectedFolder);
                imageWallpaper.lastFolder = folder;
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
            currentFolder: imageWallpaper.lastFolder
            nameFilters: imageWallpaper.nameFilters()
            fileMode: QtDialogs.FileDialog.OpenFiles
            options: QtDialogs.FileDialog.ReadOnly
            title: i18ndc("plasma_wallpaper_org.kde.image", "@title:window", "Open Image")
        }
    }

    Component {
        id: addFolderDialog

        QtDialogs.FolderDialog {
            id: folderDialog
            visible: dialogLoader.status === Loader.Ready
            currentFolder: imageWallpaper.lastFolder
            options: QtDialogs.FolderDialog.ReadOnly
            title: i18nc("@title:window", "Directory with the wallpaper to show slides from")
        }
    }
}
