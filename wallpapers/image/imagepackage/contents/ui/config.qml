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
import QtQuick.Dialogs 1.1 as QtDialogs
import QtQuick.Layouts 1.0
import QtQuick.Window 2.0 // for Screen
//We need units from it
import org.kde.plasma.core 2.0 as Plasmacore
import org.kde.plasma.wallpapers.image 2.0 as Wallpaper
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kconfig 1.0 // for KAuthorized
import org.kde.draganddrop 2.0 as DragDrop

ColumnLayout {
    id: root
    property alias cfg_Color: colorDialog.color
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
        QtControls.Label {
            id: positionLabel
            width: formAlignment - units.largeSpacing
            anchors {
                verticalCenter: resizeComboBox.verticalCenter
            }
            text: i18nd("plasma_applet_org.kde.image", "Positioning:")
            horizontalAlignment: Text.AlignRight
        }
        QtControls.ComboBox {
            id: resizeComboBox
            property int textLength: 24
            width: theme.mSize(theme.defaultFont).width * textLength
            model: [
                        {
                            'label': i18nd("plasma_applet_org.kde.image", "Scaled and Cropped"),
                            'fillMode': Image.PreserveAspectCrop
                        },
                        {
                            'label': i18nd("plasma_applet_org.kde.image","Scaled"),
                            'fillMode': Image.Stretch
                        },
                        {
                            'label': i18nd("plasma_applet_org.kde.image","Scaled, Keep Proportions"),
                            'fillMode': Image.PreserveAspectFit
                        },
                        {
                            'label': i18nd("plasma_applet_org.kde.image", "Centered"),
                            'fillMode': Image.Pad
                        },
                        {
                            'label': i18nd("plasma_applet_org.kde.image","Tiled"),
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

    QtControls.ExclusiveGroup { id: backgroundGroup }

    Row {
        id: blurRow
        spacing: units.largeSpacing / 2
        visible: cfg_FillMode === Image.PreserveAspectFit || cfg_FillMode === Image.Pad
        QtControls.Label {
            id: blurLabel
            width: formAlignment - units.largeSpacing
            anchors.verticalCenter: blurRadioButton.verticalCenter
            horizontalAlignment: Text.AlignRight
            text: i18nd("plasma_applet_org.kde.image", "Background:")
        }
        QtControls.RadioButton {
            id: blurRadioButton
            text: i18nd("plasma_applet_org.kde.image", "Blur")
            exclusiveGroup: backgroundGroup
        }
    }

    QtDialogs.ColorDialog {
        id: colorDialog
        modality: Qt.WindowModal
        showAlphaChannel: false
        title: i18nd("plasma_applet_org.kde.image", "Select Background Color")
    }

    Row {
        id: colorRow
        visible: cfg_FillMode === Image.PreserveAspectFit || cfg_FillMode === Image.Pad
        spacing: units.largeSpacing / 2
        QtControls.Label {
            width: formAlignment - units.largeSpacing
        }
        QtControls.RadioButton {
            id: colorRadioButton
            text: i18nd("plasma_applet_org.kde.image", "Solid color")
            exclusiveGroup: backgroundGroup
            checked: !cfg_Blur
        }
        QtControls.Button {
            id: colorButton
            width: units.gridUnit * 3
            text: " " // needed to it gets a proper height...
            onClicked: colorDialog.open()

            Rectangle {
                id: colorRect
                anchors.centerIn: parent
                width: parent.width - 2 * units.smallSpacing
                height: theme.mSize(theme.defaultFont).height
                color: colorDialog.color
            }
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
                QtControls.Label {
                    Layout.minimumWidth: formAlignment - units.largeSpacing
                    anchors.verticalCenter: parent.verticalCenter
                    horizontalAlignment: Text.AlignRight
                    text: i18nd("plasma_applet_org.kde.image","Change every:")
                }
                QtControls.SpinBox {
                    id: hoursInterval
                    anchors.verticalCenter: parent.verticalCenter
                    Layout.minimumWidth: textMetrics.width + units.gridUnit
                    width: units.gridUnit * 3
                    decimals: 0
                    value: root.hoursIntervalValue
                    minimumValue: 0
                    maximumValue: 24
                    onValueChanged: cfg_SlideInterval = hoursInterval.value * 3600 + minutesInterval.value * 60 + secondsInterval.value
                }
                QtControls.Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: i18nd("plasma_applet_org.kde.image","Hours")
                }
                Item {
                    Layout.preferredWidth: units.gridUnit
                }
                QtControls.SpinBox {
                    id: minutesInterval
                    anchors.verticalCenter: parent.verticalCenter
                    Layout.minimumWidth: textMetrics.width + units.gridUnit
                    width: units.gridUnit * 3
                    decimals: 0
                    value: root.minutesIntervalValue
                    minimumValue: 0
                    maximumValue: 60
                    onValueChanged: cfg_SlideInterval = hoursInterval.value * 3600 + minutesInterval.value * 60 + secondsInterval.value
                }
                QtControls.Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: i18nd("plasma_applet_org.kde.image","Minutes")
                }
                Item {
                    Layout.preferredWidth: units.gridUnit
                }
                QtControls.SpinBox {
                    id: secondsInterval
                    anchors.verticalCenter: parent.verticalCenter
                    Layout.minimumWidth: textMetrics.width + units.gridUnit
                    width: units.gridUnit * 3
                    decimals: 0
                    value: root.secondsIntervalValue
                    minimumValue: root.hoursIntervalValue === 0 && root.minutesIntervalValue === 0 ? 1 : 0
                    maximumValue: 60
                    onValueChanged: cfg_SlideInterval = hoursInterval.value * 3600 + minutesInterval.value * 60 + secondsInterval.value
                }
                QtControls.Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: i18nd("plasma_applet_org.kde.image","Seconds")
                }
            }
            QtControls.ScrollView {
                Layout.fillHeight: true;
                Layout.fillWidth: true
                frameVisible: true
                ListView {
                    id: slidePathsView
                    anchors.margins: 4
                    model: imageWallpaper.slidePaths
                    delegate: QtControls.Label {
                        text: modelData
                        width: slidePathsView.width
                        height: Math.max(paintedHeight, removeButton.height);
                        QtControls.ToolButton {
                            id: removeButton
                            anchors {
                                verticalCenter: parent.verticalCenter
                                right: parent.right
                            }
                            iconName: "list-remove"
                            onClicked: imageWallpaper.removeSlidePath(modelData);
                        }
                    }
                }
            }
        }
    }

    Component {
        id: thumbnailsComponent
        QtControls.ScrollView {
            anchors.fill: parent

            frameVisible: true
            highlightOnFocus: true;

            Component.onCompleted: {
                //replace the current binding on the scrollbar that makes it visible when content doesn't fit

                //otherwise we adjust gridSize when we hide the vertical scrollbar and
                //due to layouting that can make everything adjust which changes the contentWidth/height which
                //changes our scrollbars and we continue being stuck in a loop

                //looks better to not have everything resize anyway.
                //BUG: 336301
                __verticalScrollBar.visible = true
            }

            GridView {
                id: wallpapersGrid
                model: imageWallpaper.wallpaperModel
                currentIndex: -1
                focus: true

                cellWidth: Math.floor(wallpapersGrid.width / Math.max(Math.floor(wallpapersGrid.width / (units.gridUnit*12)), 3))
                cellHeight: Math.round(cellWidth / (imageWallpaper.targetSize.width / imageWallpaper.targetSize.height))

                anchors.margins: 4
                boundsBehavior: Flickable.StopAtBounds

                delegate: WallpaperDelegate {
                    color: cfg_Color
                }

                onContentHeightChanged: {
                    wallpapersGrid.currentIndex = imageWallpaper.wallpaperModel.indexOf(cfg_Image);
                    wallpapersGrid.positionViewAtIndex(wallpapersGrid.currentIndex, GridView.Visible)
                }

                Keys.onPressed: {
                    if (count < 1) {
                        return;
                    }

                    if (event.key == Qt.Key_Home) {
                        currentIndex = 0;
                    } else if (event.key == Qt.Key_End) {
                        currentIndex = count - 1;
                    }
                }

                Keys.onLeftPressed: moveCurrentIndexLeft()
                Keys.onRightPressed: moveCurrentIndexRight()
                Keys.onUpPressed: moveCurrentIndexUp()
                Keys.onDownPressed: moveCurrentIndexDown()

                Connections {
                    target: imageWallpaper
                    onCustomWallpaperPicked: {
                        wallpapersGrid.currentIndex = 0;
                        cfg_Image = path;
                    }
                }

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
        anchors {
            right: parent.right
        }
        QtControls.Button {
            visible: (configDialog.currentWallpaper == "org.kde.slideshow")
            iconName: "list-add"
            text: i18nd("plasma_applet_org.kde.image","Add Folder...")
            onClicked: imageWallpaper.showAddSlidePathsDialog()
        }
        QtControls.Button {
            visible: (configDialog.currentWallpaper == "org.kde.image")
            iconName: "list-add"
            text: i18nd("plasma_applet_org.kde.image","Add Image...")
            onClicked: imageWallpaper.showFileDialog();
        }
        QtControls.Button {
            iconName: "get-hot-new-stuff"
            text: i18nd("plasma_applet_org.kde.image","Get New Wallpapers...")
            visible: KAuthorized.authorize("ghns")
            onClicked: imageWallpaper.getNewWallpaper(this);
        }
    }
}
