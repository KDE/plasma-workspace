/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QtControls2
import Qt5Compat.GraphicalEffects

import org.kde.kirigami as Kirigami
import org.kde.kquickcontrolsaddons
import org.kde.kcmutils as KCM

KCM.GridDelegate {
    id: wallpaperDelegate

    property alias color: backgroundRect.color
    opacity: model.pendingDeletion ? 0.5 : 1
    scale: index, 1 // Workaround for https://bugreports.qt.io/browse/QTBUG-107458

    text: model.display
    subtitle: model.author

    hoverEnabled: true

    actions: [
        Kirigami.Action {
            icon.name: "document-open-folder"
            tooltip: i18nd("plasma_wallpaper_org.kde.image", "Open Containing Folder")
            onTriggered: imageModel.openContainingFolder(index)
        },
        Kirigami.Action {
            icon.name: "edit-undo"
            visible: model.pendingDeletion
            tooltip: i18nd("plasma_wallpaper_org.kde.image", "Restore wallpaper")
            onTriggered: model.pendingDeletion = false
        },
        Kirigami.Action {
            icon.name: "edit-delete"
            tooltip: i18nd("plasma_wallpaper_org.kde.image", "Remove Wallpaper")
            visible: model.removable && !model.pendingDeletion && configDialog.currentWallpaper == "org.kde.image"
            onTriggered: {
                model.pendingDeletion = true;

                if (wallpapersGrid.view.currentIndex === index) {
                    const newIndex = (index + 1) % (imageModel.count - 1);
                    wallpapersGrid.view.itemAtIndex(newIndex).clicked();
                }
                root.configurationChanged(); // BUG 438585
            }
        }
    ]

    thumbnail: Rectangle {
        id: backgroundRect
        color: cfg_Color
        anchors.fill: parent

        Kirigami.Icon {
            anchors.centerIn: parent
            width: Kirigami.Units.iconSizes.large
            height: width
            source: "view-preview"
            visible: !walliePreview.visible
        }

        QPixmapItem {
            id: blurBackgroundSource
            visible: cfg_Blur
            anchors.fill: parent
            smooth: true
            pixmap: model.screenshot
            fillMode: QPixmapItem.PreserveAspectCrop
        }

        FastBlur {
            visible: cfg_Blur
            anchors.fill: parent
            source: blurBackgroundSource
            radius: 4
        }

        QPixmapItem {
            id: walliePreview
            anchors.fill: parent
            visible: model.screenshot !== null
            smooth: true
            pixmap: model.screenshot
            fillMode: {
                if (cfg_FillMode === Image.Stretch) {
                    return QPixmapItem.Stretch;
                } else if (cfg_FillMode === Image.PreserveAspectFit) {
                    return QPixmapItem.PreserveAspectFit;
                } else if (cfg_FillMode === Image.PreserveAspectCrop) {
                    return QPixmapItem.PreserveAspectCrop;
                } else if (cfg_FillMode === Image.Tile) {
                    return QPixmapItem.Tile;
                } else if (cfg_FillMode === Image.TileVertically) {
                    return QPixmapItem.TileVertically;
                } else if (cfg_FillMode === Image.TileHorizontally) {
                    return QPixmapItem.TileHorizontally;
                }
                return QPixmapItem.PreserveAspectFit;
            }
        }
        QtControls2.CheckBox {
            visible: configDialog.currentWallpaper === "org.kde.slideshow"
            anchors.right: parent.right
            anchors.top: parent.top
            checked: visible ? model.checked : false
            onToggled: model.checked = checked
        }

        Behavior on color {
            ColorAnimation {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutQuad
            }
        }
    }

    Behavior on opacity {
        OpacityAnimator {
            duration: Kirigami.Units.longDuration
            easing.type: Easing.InOutQuad
        }
    }

    onClicked: {
        if (configDialog.currentWallpaper === "org.kde.image") {
            cfg_Image = model.packageName || model.path;
            if (typeof wallpaper !== "undefined") {
                wallpaper.configuration.PreviewImage = cfg_Image;
            }
        }
        GridView.currentIndex = index;
    }
}
