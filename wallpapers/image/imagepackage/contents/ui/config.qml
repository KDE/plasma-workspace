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

import QtQuick 2.0
import QtQuick.Controls 1.0 as QtControls
import QtQuick.Dialogs 1.1 as QtDialogs
import QtQuick.Layouts 1.0
//We need units from it
import org.kde.plasma.core 2.0 as Plasmacore
import org.kde.plasma.wallpapers.image 2.0 as Wallpaper
import org.kde.kquickcontrolsaddons 2.0

ColumnLayout {
    id: root
    property alias cfg_Color: colorDialog.color
    property string cfg_Image
    property int cfg_FillMode
    property var cfg_SlidePaths: ""
    property int cfg_SlideInterval: 0
    signal restoreIndex(int count)

    function saveConfig() {
        root.restoreIndex(imageWallpaper.wallpaperModel.count)
        imageWallpaper.commitDeletion();
    }

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

    QtDialogs.ColorDialog {
        id: colorDialog
        modality: Qt.WindowModal
        showAlphaChannel: false
        title: i18nd("plasma_applet_org.kde.image", "Select Background Color")
    }

    Row {
        id: colorRow
        spacing: units.largeSpacing / 2
        visible: ~[2,3].indexOf(resizeComboBox.currentIndex)

        QtControls.Label {
            width: formAlignment - units.largeSpacing
            anchors.verticalCenter: colorButton.verticalCenter
            horizontalAlignment: Text.AlignRight
            text: i18nd("plasma_applet_org.kde.image", "Background Color:")
        }
        QtControls.Button {
            id: colorButton
            width: units.gridUnit * 3
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
                    text: i18nd("plasma_applet_org.kde.image","Change every:")
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
                    text: i18nd("plasma_applet_org.kde.image","Hours")
                }
                Item {
                    width: units.gridUnit
                    height: units.gridUnit
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
                    text: i18nd("plasma_applet_org.kde.image","Minutes")
                }
                Item {
                    width: units.gridUnit
                    height: units.gridUnit
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
                    text: i18nd("plasma_applet_org.kde.image","Seconds")
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
                cellHeight: cellWidth / (imageWallpaper.width / imageWallpaper.height)

                anchors.margins: 4
                boundsBehavior: Flickable.StopAtBounds

                delegate: WallpaperDelegate {
                    color: colorRow.visible ? cfg_Color : "black"
                }

                Connections {
                    target: root
                    onRestoreIndex: {
                        wallpapersGrid.currentIndex = wallpapersGrid.currentIndex - count
                    }
                }

                Timer {
                    id: makeCurrentTimer
                    interval: 100
                    repeat: false
                    property string pendingIndex
                    onTriggered: {
                        wallpapersGrid.currentIndex = pendingIndex
                        wallpapersGrid.forceActiveFocus();
                    }
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
                    onCustomWallpaperPicked: wallpapersGrid.currentIndex = 0
                }

            }
        }
    }

    Loader {
        Layout.fillWidth: true
        Layout.fillHeight: true
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
            text: i18nd("plasma_applet_org.kde.image","Add Folder")
            onClicked: imageWallpaper.showAddSlidePathsDialog()
        }
        QtControls.Button {
            visible: (configDialog.currentWallpaper == "org.kde.image")
            iconName: "document-open-folder"
            text: i18nd("plasma_applet_org.kde.image","Open...")
            onClicked: imageWallpaper.showFileDialog();
        }
        QtControls.Button {
            iconName: "get-hot-new-stuff"
            text: i18nd("plasma_applet_org.kde.image","Get New Wallpapers...")
            onClicked: imageWallpaper.getNewWallpaper();
        }
    }
}
