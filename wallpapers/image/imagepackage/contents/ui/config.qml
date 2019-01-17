/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
 *  Copyright 2014 Kai Uwe Broulik <kde@privat.broulik.de>
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

import QtQuick 2.5
import QtQuick.Controls 1.0 as QtControls
import QtQuick.Controls 2.3 as QtControls2
import QtQuick.Layouts 1.0
import QtQuick.Window 2.0 // for Screen
//We need units from it
import org.kde.plasma.core 2.0 as Plasmacore
import org.kde.plasma.wallpapers.image 2.0 as Wallpaper
import org.kde.kquickcontrols 2.0 as KQuickControls
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kconfig 1.0 // for KAuthorized
import org.kde.draganddrop 2.0 as DragDrop
import org.kde.kcm 1.1 as KCM
import org.kde.kirigami 2.5 as Kirigami

ColumnLayout {
    id: root
    property alias cfg_Color: colorButton.color
    property string cfg_Image
    property int cfg_FillMode
    property alias cfg_Blur: blurRadioButton.checked
    property var cfg_SlidePaths: ""
    property int cfg_SlideInterval: 0

    function saveConfig() {
        imageWallpaper.commitDeletion();
    }

    SystemPalette {
        id: syspal
    }

    Wallpaper.Image {
        id: imageWallpaper
        targetSize: {
            if (typeof plasmoid !== "undefined") {
                return Qt.size(plasmoid.width, plasmoid.height)
            }
            // Lock screen configuration case
            return Qt.size(Screen.width, Screen.height)
        }
        onSlidePathsChanged: cfg_SlidePaths = slidePaths
    }

    onCfg_SlidePathsChanged: {
        imageWallpaper.slidePaths = cfg_SlidePaths
    }

    property int hoursIntervalValue: Math.floor(cfg_SlideInterval / 3600)
    property int minutesIntervalValue: Math.floor(cfg_SlideInterval % 3600) / 60
    property int secondsIntervalValue: cfg_SlideInterval % 3600 % 60

    //Rectangle { color: "orange"; x: formAlignment; width: formAlignment; height: 20 }

    TextMetrics {
        id: textMetrics
        text: "00"
    }

    Row {
        //x: formAlignment - positionLabel.paintedWidth
        spacing: units.largeSpacing / 2
        QtControls2.Label {
            id: positionLabel
            width: formAlignment - units.largeSpacing
            anchors {
                verticalCenter: resizeComboBox.verticalCenter
            }
            text: i18nd("plasma_wallpaper_org.kde.image", "Positioning:")
            horizontalAlignment: Text.AlignRight
        }

        // TODO: port to QQC2 version once we've fixed https://bugs.kde.org/show_bug.cgi?id=403153
        QtControls.ComboBox {
            id: resizeComboBox
            TextMetrics {
                id: resizeTextMetrics
                text: resizeComboBox.currentText
            }
            width: resizeTextMetrics.width + Kirigami.Units.smallSpacing * 2 + Kirigami.Units.gridUnit * 2
            model: [
                        {
                            'label': i18nd("plasma_wallpaper_org.kde.image", "Scaled and Cropped"),
                            'fillMode': Image.PreserveAspectCrop
                        },
                        {
                            'label': i18nd("plasma_wallpaper_org.kde.image","Scaled"),
                            'fillMode': Image.Stretch
                        },
                        {
                            'label': i18nd("plasma_wallpaper_org.kde.image","Scaled, Keep Proportions"),
                            'fillMode': Image.PreserveAspectFit
                        },
                        {
                            'label': i18nd("plasma_wallpaper_org.kde.image", "Centered"),
                            'fillMode': Image.Pad
                        },
                        {
                            'label': i18nd("plasma_wallpaper_org.kde.image","Tiled"),
                            'fillMode': Image.Tile
                        }
                    ]

            textRole: "label"
            onCurrentIndexChanged: cfg_FillMode = model[currentIndex]["fillMode"]
            Component.onCompleted: setMethod();

            function setMethod() {
                for (var i = 0; i < model.length; i++) {
                    if (model[i]["fillMode"] == wallpaper.configuration.FillMode) {
                        resizeComboBox.currentIndex = i;
                        var tl = model[i]["label"].length;
                        //resizeComboBox.textLength = Math.max(resizeComboBox.textLength, tl+5);
                    }
                }
            }
        }
    }

    QtControls2.ButtonGroup { id: backgroundGroup }

    Row {
        id: blurRow
        spacing: units.largeSpacing / 2
        visible: cfg_FillMode === Image.PreserveAspectFit || cfg_FillMode === Image.Pad
        QtControls2.Label {
            id: blurLabel
            width: formAlignment - units.largeSpacing
            anchors.verticalCenter: blurRadioButton.verticalCenter
            horizontalAlignment: Text.AlignRight
            text: i18nd("plasma_wallpaper_org.kde.image", "Background:")
        }
        QtControls2.RadioButton {
            id: blurRadioButton
            text: i18nd("plasma_wallpaper_org.kde.image", "Blur")
            QtControls2.ButtonGroup.group: backgroundGroup
        }
    }

    Row {
        id: colorRow
        visible: cfg_FillMode === Image.PreserveAspectFit || cfg_FillMode === Image.Pad
        spacing: units.largeSpacing / 2
        QtControls2.Label {
            width: formAlignment - units.largeSpacing
        }
        QtControls2.RadioButton {
            id: colorRadioButton
            text: i18nd("plasma_wallpaper_org.kde.image", "Solid color")
            QtControls2.ButtonGroup.group: backgroundGroup
            checked: !cfg_Blur
        }
        KQuickControls.ColorButton {
            id: colorButton
            dialogTitle: i18nd("plasma_wallpaper_org.kde.image", "Select Background Color")
        }
    }

    Component {
        id: foldersComponent
        ColumnLayout {
            anchors.fill: parent
            Connections {
                target: root
                onHoursIntervalValueChanged: hoursInterval.value = root.hoursIntervalValue
                onMinutesIntervalValueChanged: minutesInterval.value = root.minutesIntervalValue
                onSecondsIntervalValueChanged: secondsInterval.value = root.secondsIntervalValue
            }
            //FIXME: there should be only one spinbox: QtControls spinboxes are still too limited for it tough
            RowLayout {
                Layout.fillWidth: true
                spacing: units.largeSpacing / 2
                QtControls2.Label {
                    Layout.minimumWidth: formAlignment - units.largeSpacing
                    horizontalAlignment: Text.AlignRight
                    text: i18nd("plasma_wallpaper_org.kde.image","Change every:")
                }
                QtControls2.SpinBox {
                    id: hoursInterval
                    Layout.minimumWidth: textMetrics.width + units.gridUnit
                    width: units.gridUnit * 3
                    value: root.hoursIntervalValue
                    from: 0
                    to: 24
                    editable: true
                    onValueChanged: cfg_SlideInterval = hoursInterval.value * 3600 + minutesInterval.value * 60 + secondsInterval.value
                }
                QtControls2.Label {
                    text: i18nd("plasma_wallpaper_org.kde.image","Hours")
                }
                Item {
                    Layout.preferredWidth: units.gridUnit
                }
                QtControls2.SpinBox {
                    id: minutesInterval
                    Layout.minimumWidth: textMetrics.width + units.gridUnit
                    width: units.gridUnit * 3
                    value: root.minutesIntervalValue
                    from: 0
                    to: 60
                    editable: true
                    onValueChanged: cfg_SlideInterval = hoursInterval.value * 3600 + minutesInterval.value * 60 + secondsInterval.value
                }
                QtControls2.Label {
                    text: i18nd("plasma_wallpaper_org.kde.image","Minutes")
                }
                Item {
                    Layout.preferredWidth: units.gridUnit
                }
                QtControls2.SpinBox {
                    id: secondsInterval
                    Layout.minimumWidth: textMetrics.width + units.gridUnit
                    width: units.gridUnit * 3
                    value: root.secondsIntervalValue
                    from: root.hoursIntervalValue === 0 && root.minutesIntervalValue === 0 ? 1 : 0
                    to: 60
                    editable: true
                    onValueChanged: cfg_SlideInterval = hoursInterval.value * 3600 + minutesInterval.value * 60 + secondsInterval.value
                }
                QtControls2.Label {
                    text: i18nd("plasma_wallpaper_org.kde.image","Seconds")
                }
            }
            QtControls2.ScrollView {
                id: foldersScroll
                Layout.fillHeight: true;
                Layout.fillWidth: true
                Component.onCompleted: foldersScroll.background.visible = true;
                ListView {
                    id: slidePathsView
                    anchors.margins: 4
                    model: imageWallpaper.slidePaths
                    delegate: QtControls2.Label {
                        text: modelData
                        width: slidePathsView.width
                        height: Math.max(paintedHeight, removeButton.height);
                        QtControls2.ToolButton {
                            id: removeButton
                            anchors {
                                verticalCenter: parent.verticalCenter
                                right: parent.right
                            }
                            icon.name: "list-remove"
                            onClicked: imageWallpaper.removeSlidePath(modelData);
                        }
                    }
                }
            }
        }
    }

    Component {
        id: thumbnailsComponent
        KCM.GridView {
            id: wallpapersGrid
            anchors.fill: parent

            //that min is needed as the module will be populated in an async way
            //and only on demand so we can't ensure it already exists
            view.currentIndex: Math.min(imageWallpaper.wallpaperModel.indexOf(cfg_Image), imageWallpaper.wallpaperModel.count-1)
            //kill the space for label under thumbnails
            view.model: imageWallpaper.wallpaperModel
            view.delegate: WallpaperDelegate {
                color: cfg_Color
            }
        }
    }

    DragDrop.DropArea {
        Layout.fillWidth: true
        Layout.fillHeight: true

        onDragEnter: {
            if (!event.mimeData.hasUrls) {
                event.ignore();
            }
        }
        onDrop: {
            event.mimeData.urls.forEach(function (url) {
                if (url.indexOf("file://") === 0) {
                    var path = url.substr(7); // 7 is length of "file://"
                    if (configDialog.currentWallpaper === "org.kde.image") {
                        imageWallpaper.addUsersWallpaper(path);
                    } else {
                        imageWallpaper.addSlidePath(path);
                    }
                }
            });
        }

        Loader {
            anchors.fill: parent
            sourceComponent: (configDialog.currentWallpaper == "org.kde.image") ? thumbnailsComponent : foldersComponent
        }
    }

    RowLayout {
        id: buttonsRow
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        QtControls2.Button {
            visible: (configDialog.currentWallpaper == "org.kde.slideshow")
            icon.name: "list-add"
            text: i18nd("plasma_wallpaper_org.kde.image","Add Folder...")
            onClicked: imageWallpaper.showAddSlidePathsDialog()
        }
        QtControls2.Button {
            visible: (configDialog.currentWallpaper == "org.kde.image")
            icon.name: "list-add"
            text: i18nd("plasma_wallpaper_org.kde.image","Add Image...")
            onClicked: imageWallpaper.showFileDialog();
        }
        QtControls2.Button {
            icon.name: "get-hot-new-stuff"
            text: i18nd("plasma_wallpaper_org.kde.image","Get New Wallpapers...")
            visible: KAuthorized.authorize("ghns")
            onClicked: imageWallpaper.getNewWallpaper(this);
        }
    }
}
