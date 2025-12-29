/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QtControls2
import Qt5Compat.GraphicalEffects

import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

KCM.GridDelegate {
    id: wallpaperDelegate

    property alias color: backgroundRect.color
    property alias previewSize: previewImage.sourceSize
    property string key: model.source
    property list<string> selectors: model.selectors
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
            icon.name: "edit-delete-remove"
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
            visible: previewImage.status != Image.Ready
        }

        FastBlur {
            id: fastBlur
            visible: cfg_Blur
            anchors.fill: parent
            radius: 4
            source: Image {
                asynchronous: true
                retainWhileLoading: true
                cache: false
                fillMode: Image.PreserveAspectCrop
                source: fastBlur.visible ? previewImage.source : ""
                sourceSize: previewImage.sourceSize
                visible: false
            }
        }

        Image {
            id: previewImage
            anchors.fill: parent
            asynchronous: true
            retainWhileLoading: true
            cache: false
            fillMode: cfg_FillMode
            source: model.preview
        }

        QtControls2.CheckBox {
            visible: configDialog.currentWallpaper === "org.kde.slideshow"
            anchors.left: parent.left
            anchors.margins: Kirigami.Units.smallSpacing
            anchors.top: parent.top
            checked: visible ? model.checked : false
            onToggled: model.checked = checked
        }

        Kirigami.Icon {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Kirigami.Units.smallSpacing
            visible: model.selectors.includes("day-night")
            source: "lighttable"
            isMask: true
            color: "white"
            width: Kirigami.Units.iconSizes.medium
            height: width
        }

        Behavior on color {
            ColorAnimation {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutQuad
            }
        }
    }

    Behavior on opacity {
        NumberAnimation {
            duration: Kirigami.Units.longDuration
            easing.type: Easing.InOutQuad
        }
    }

    onClicked: {
        if (configDialog.currentWallpaper === "org.kde.image") {
            root.selectWallpaper(key, selectors);
        } else {
            model.checked = !model.checked
        }
        GridView.currentIndex = index;
    }
}
