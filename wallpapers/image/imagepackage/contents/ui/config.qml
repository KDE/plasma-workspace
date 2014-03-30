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
import QtQuick.Controls 1.0 as QtControls
import QtQuick.Layouts 1.0
import org.kde.plasma.wallpapers.image 2.0 as Wallpaper
import org.kde.kquickcontrolsaddons 2.0

ColumnLayout {
    id: root
    property alias cfg_Color: picker.color
    property string cfg_Image
    property int cfg_FillMode
    property var cfg_SlidePaths: ""
    property int cfg_SlideInterval: 0

    spacing: units.largeSpacing / 2

    Wallpaper.Image {
        id: imageWallpaper
        width: wallpaper.configuration.width
        height: wallpaper.configuration.height
        onSlidePathsChanged: cfg_SlidePaths = slidePaths
    }

    onCfg_SlidePathsChanged: {
        imageWallpaper.slidePaths = cfg_SlidePaths
    }

    property int hoursIntervalValue
    property int minutesIntervalValue
    property int secondsIntervalValue

    onCfg_SlideIntervalChanged: {
        hoursIntervalValue = Math.floor(cfg_SlideInterval / 3600)
        minutesIntervalValue = Math.floor(cfg_SlideInterval % 3600) / 60
        secondsIntervalValue = cfg_SlideInterval % 3600 % 60
    }
    
    //Rectangle { color: "orange"; x: formAlignment; width: formAlignment; height: 20 }

    Row {
        //x: formAlignment - positionLabel.paintedWidth
        spacing: units.largeSpacing / 2
        QtControls.Label {
            id: positionLabel
            width: formAlignment - units.largeSpacing
            anchors {
                verticalCenter: resizeComboBox.verticalCenter
            }
            text: i18nc("Label for positioning combo", "Positioning:")
            horizontalAlignment: Text.AlignRight
        }
        QtControls.ComboBox {
            id: resizeComboBox
            property int textLength: 24
            width: theme.mSize(theme.defaultFont).width * textLength
            model: [
                        {
                            'label': i18n("Scaled & Cropped"),
                            'fillMode': Image.PreserveAspectCrop
                        },
                        {
                            'label': i18n("Scaled"),
                            'fillMode': Image.Stretch
                        },
                        {
                            'label': i18n("Scaled, Keep Proportions"),
                            'fillMode': Image.PreserveAspectFit
                        },
                        {
                            'label': i18n("Centered"),
                            'fillMode': Image.Pad
                        },
                        {
                            'label': i18n("Tiled"),
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

    ColorPicker {
        id: picker
        visible: ~[2,3].indexOf(resizeComboBox.currentIndex)
    }

    Component {
        id: foldersComponent
        ColumnLayout {
            anchors.fill: parent
            Connections {
                target: root
                onHoursIntervalValueChanged: hoursInterval.value = root.hoursIntervalValue
                onMinutesIntervalValueChanged: minutesInterval.value = root.minutesIntervalValue
                onSecondsIntervalValueChanged: {
                    secondsInterval.value = root.secondsIntervalValue}
            }
            Component.onCompleted: {
                hoursInterval.value = root.hoursIntervalValue
                minutesInterval.value = root.minutesIntervalValue
                secondsInterval.value = root.secondsIntervalValue
            }
            //FIXME: there should be only one spinbox: QtControls spinboxes are still too limited for it tough
            Row {
                spacing: units.largeSpacing / 2
                QtControls.Label {
                    width: formAlignment - units.largeSpacing
                    anchors.verticalCenter: parent.verticalCenter
                    horizontalAlignment: Text.AlignRight
                    text: i18n("Change every:")
                }
                QtControls.SpinBox {
                    id: hoursInterval
                    anchors.verticalCenter: parent.verticalCenter
                    width: units.gridUnit * 3
                    decimals: 0
                    minimumValue: 0
                    maximumValue: 24
                    onValueChanged: cfg_SlideInterval = hoursInterval.value * 3600 + minutesInterval.value * 60 + secondsInterval.value
                }
                QtControls.Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: i18n("Hours")
                }
                QtControls.SpinBox {
                    id: minutesInterval
                    anchors.verticalCenter: parent.verticalCenter
                    width: units.gridUnit * 3
                    decimals: 0
                    minimumValue: 0
                    maximumValue: 60
                    onValueChanged: cfg_SlideInterval = hoursInterval.value * 3600 + minutesInterval.value * 60 + secondsInterval.value
                }
                QtControls.Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: i18n("Minutes")
                }
                QtControls.SpinBox {
                    id: secondsInterval
                    anchors.verticalCenter: parent.verticalCenter
                    width: units.gridUnit * 3
                    decimals: 0
                    minimumValue: minutesInterval.value == 0 && hoursInterval.value == 0 ? 1 : 0
                    maximumValue: 60
                    onValueChanged: cfg_SlideInterval = hoursInterval.value * 3600 + minutesInterval.value * 60 + secondsInterval.value
                }
                QtControls.Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: i18n("Seconds")
                }
            }
            QtControls.ScrollView {
                Layout.fillHeight: true;
                anchors {
                    left: parent.left
                    right: parent.right
                }
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

            GridView {
                id: wallpapersGrid
                model: imageWallpaper.wallpaperModel
                currentIndex: -1

                cellWidth: wallpapersGrid.width / 3
                cellHeight: cellWidth / units.displayAspectRatio

                anchors.margins: 4
                boundsBehavior: Flickable.StopAtBounds

                highlight: Rectangle {
                    radius: 3
                    color: syspal.highlight
                }
                delegate: WallpaperDelegate {}
                Timer {
                    id: makeCurrentTimer
                    interval: 100
                    repeat: false
                    property string pendingIndex
                    onTriggered: {
                        wallpapersGrid.currentIndex = pendingIndex
                    }
                }

                Connections {
                    target: imageWallpaper
                    onCustomWallpaperPicked: wallpapersGrid.currentIndex = 0
                }
            }
        }
    }

    Loader {
        Layout.fillHeight: true;
        anchors {
            left: parent.left
            right: parent.right
        }
        height: units.gridUnit * 30

        sourceComponent: (configDialog.currentWallpaper == "org.kde.image") ? thumbnailsComponent : foldersComponent
    }

    RowLayout {
        id: buttonsRow
        anchors {
            right: parent.right
        }
        QtControls.Button {
            visible: (configDialog.currentWallpaper == "org.kde.slideshow")
            iconName: "list-add"
            text: i18n("Add Folder")
            onClicked: imageWallpaper.showAddSlidePathsDialog()
        }
        QtControls.Button {
            visible: (configDialog.currentWallpaper == "org.kde.image")
            iconName: "document-open-folder"
            text: i18n("Open...")
            onClicked: imageWallpaper.showFileDialog();
        }
        QtControls.Button {
            iconName: "bookmarks"
            text: i18n("Download Wallpapers")
            onClicked: imageWallpaper.getNewWallpaper();
        }
    }
}