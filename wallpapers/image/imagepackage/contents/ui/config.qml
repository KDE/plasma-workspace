/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
 *  Copyright 2014 Kai Uwe Broulik <kde@privat.broulik.de>
 *  Copyright 2019 David Redondo <kde@david-redondo.de>
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
import QtQuick.Controls 2.5 as QtControls2
import QtQuick.Layouts 1.0
import QtQuick.Window 2.0 // for Screen
import org.kde.plasma.wallpapers.image 2.0 as Wallpaper
import org.kde.kquickcontrols 2.0 as KQuickControls
import org.kde.kquickcontrolsaddons 2.0
import org.kde.newstuff 1.62 as NewStuff
import org.kde.draganddrop 2.0 as DragDrop
import org.kde.kcm 1.5 as KCM
import org.kde.kirigami 2.12 as Kirigami

ColumnLayout {
    id: root
    property alias cfg_Color: colorButton.color
    property color cfg_ColorDefault
    property string cfg_Image
    property string cfg_ImageDefault
    property int cfg_FillMode
    property int cfg_FillModeDefault
    property int cfg_SlideshowMode
    property int cfg_SlideshowModeDefault
    property alias cfg_Blur: blurRadioButton.checked
    property bool cfg_BlurDefault
    property string cfg_SlidePaths: ""
    property string cfg_SlidePathsDefault: ""
    property int cfg_SlideInterval: 0
    property int cfg_SlideIntervalDefault: 0
    property var cfg_UncheckedSlides: []
    property var cfg_UncheckedSlidesDefault: []

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
        onUncheckedSlidesChanged: cfg_UncheckedSlides = uncheckedSlides
        onSlideshowModeChanged: cfg_SlideshowMode = slideshowMode
    }

    onCfg_FillModeChanged: {
        resizeComboBox.setMethod()
    }

    onCfg_SlidePathsChanged: {
        imageWallpaper.slidePaths = cfg_SlidePaths
    }
    onCfg_UncheckedSlidesChanged: {
        imageWallpaper.uncheckedSlides = cfg_UncheckedSlides
    }

    onCfg_SlideshowModeChanged: {
        imageWallpaper.slideshowMode = cfg_SlideshowMode
    }

    property int hoursIntervalValue: Math.floor(cfg_SlideInterval / 3600)
    property int minutesIntervalValue: Math.floor(cfg_SlideInterval % 3600) / 60
    property int secondsIntervalValue: cfg_SlideInterval % 3600 % 60

    property int hoursIntervalValueDefault: Math.floor(cfg_SlideIntervalDefault / 3600)
    property int minutesIntervalValueDefault: Math.floor(cfg_SlideIntervalDefault % 3600) / 60
    property int secondsIntervalValueDefault: cfg_SlideIntervalDefault % 3600 % 60

    //Rectangle { color: "orange"; x: formAlignment; width: formAlignment; height: 20 }

    Kirigami.FormLayout {
        twinFormLayouts: parentLayout
        QtControls2.ComboBox {
            id: resizeComboBox
            Kirigami.FormData.label: i18nd("plasma_wallpaper_org.kde.image", "Positioning:")
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

            KCM.SettingHighlighter {
                highlight: cfg_FillModeDefault != cfg_FillMode
            }

            function setMethod() {
                for (var i = 0; i < model.length; i++) {
                    if (model[i]["fillMode"] === root.cfg_FillMode) {
                        resizeComboBox.currentIndex = i;
                        var tl = model[i]["label"].length;
                        //resizeComboBox.textLength = Math.max(resizeComboBox.textLength, tl+5);
                    }
                }
            }
        }

        QtControls2.ComboBox {
            id: slideshowComboBox
            visible: configDialog.currentWallpaper == "org.kde.slideshow"
            Kirigami.FormData.label: i18nd("plasma_wallpaper_org.kde.image", "Order:")
            model: [
                {
                    'label': i18nd("plasma_wallpaper_org.kde.image", "Random"),
                    'slideshowMode':  Wallpaper.Image.Random
                },
                {
                    'label': i18nd("plasma_wallpaper_org.kde.image", "A to Z"),
                    'slideshowMode':  Wallpaper.Image.Alphabetical
                },
                {
                    'label': i18nd("plasma_wallpaper_org.kde.image", "Z to A"),
                    'slideshowMode':  Wallpaper.Image.AlphabeticalReversed
                },
                {
                    'label': i18nd("plasma_wallpaper_org.kde.image", "Date modified (newest first)"),
                    'slideshowMode':  Wallpaper.Image.ModifiedReversed
                },
                {
                    'label': i18nd("plasma_wallpaper_org.kde.image", "Date modified (oldest first)"),
                    'slideshowMode':  Wallpaper.Image.Modified
                }
            ]
            textRole: "label"
            onCurrentIndexChanged: {
                cfg_SlideshowMode = model[currentIndex]["slideshowMode"];
            }
            Component.onCompleted: setMethod();
            function setMethod() {
                for (var i = 0; i < model.length; i++) {
                    if (model[i]["slideshowMode"] === wallpaper.configuration.SlideshowMode) {
                        slideshowComboBox.currentIndex = i;
                    }
                }
            }

            KCM.SettingHighlighter {
                highlight: cfg_SlideshowMode != cfg_SlideshowModeDefault
            }
        }

        QtControls2.ButtonGroup { id: backgroundGroup }

        QtControls2.RadioButton {
            id: blurRadioButton
            visible: cfg_FillMode === Image.PreserveAspectFit || cfg_FillMode === Image.Pad
            Kirigami.FormData.label: i18nd("plasma_wallpaper_org.kde.image", "Background:")
            text: i18nd("plasma_wallpaper_org.kde.image", "Blur")
            QtControls2.ButtonGroup.group: backgroundGroup
        }

        RowLayout {
            id: colorRow
            visible: cfg_FillMode === Image.PreserveAspectFit || cfg_FillMode === Image.Pad
            QtControls2.RadioButton {
                id: colorRadioButton
                text: i18nd("plasma_wallpaper_org.kde.image", "Solid color")
                checked: !cfg_Blur
                QtControls2.ButtonGroup.group: backgroundGroup

                KCM.SettingHighlighter {
                    highlight: cfg_Blur != cfg_BlurDefault
                }
            }
            KQuickControls.ColorButton {
                id: colorButton
                dialogTitle: i18nd("plasma_wallpaper_org.kde.image", "Select Background Color")

                KCM.SettingHighlighter {
                    highlight: cfg_Color != cfg_ColorDefault
                }
            }
        }
    }

    Component {
        id: foldersComponent
        ColumnLayout {
            Connections {
                target: root
                function onHoursIntervalValueChanged() {hoursInterval.value = root.hoursIntervalValue}
                function onMinutesIntervalValueChanged() {minutesInterval.value = root.minutesIntervalValue}
                function onSecondsIntervalValueChanged() {secondsInterval.value = root.secondsIntervalValue}
            }
            //FIXME: there should be only one spinbox: QtControls spinboxes are still too limited for it tough
            Kirigami.FormLayout {
                twinFormLayouts: parentLayout
                RowLayout {
                    Kirigami.FormData.label: i18nd("plasma_wallpaper_org.kde.image","Change every:")
                    QtControls2.SpinBox {
                        id: hoursInterval
                        value: root.hoursIntervalValue
                        from: 0
                        to: 24
                        editable: true
                        onValueChanged: cfg_SlideInterval = hoursInterval.value * 3600 + minutesInterval.value * 60 + secondsInterval.value

                        textFromValue: function(value, locale) {
                            return i18ndp("plasma_wallpaper_org.kde.image","%1 hour", "%1 hours", value)
                        }
                        valueFromText: function(text, locale) {
                            return parseInt(text);
                        }

                        KCM.SettingHighlighter {
                            highlight: root.hoursIntervalValue != root.hoursIntervalValueDefault
                        }
                    }
                    QtControls2.SpinBox {
                        id: minutesInterval
                        value: root.minutesIntervalValue
                        from: 0
                        to: 60
                        editable: true
                        onValueChanged: cfg_SlideInterval = hoursInterval.value * 3600 + minutesInterval.value * 60 + secondsInterval.value

                        textFromValue: function(value, locale) {
                            return i18ndp("plasma_wallpaper_org.kde.image","%1 minute", "%1 minutes", value)
                        }
                        valueFromText: function(text, locale) {
                            return parseInt(text);
                        }

                        KCM.SettingHighlighter {
                            highlight: root.minutesIntervalValue != root.minutesIntervalValueDefault
                        }
                    }
                    QtControls2.SpinBox {
                        id: secondsInterval
                        value: root.secondsIntervalValue
                        from: root.hoursIntervalValue === 0 && root.minutesIntervalValue === 0 ? 1 : 0
                        to: 60
                        editable: true
                        onValueChanged: cfg_SlideInterval = hoursInterval.value * 3600 + minutesInterval.value * 60 + secondsInterval.value

                        textFromValue: function(value, locale) {
                            return i18ndp("plasma_wallpaper_org.kde.image","%1 second", "%1 seconds", value)
                        }
                        valueFromText: function(text, locale) {
                            return parseInt(text);
                        }

                        KCM.SettingHighlighter {
                            highlight: root.secondsIntervalValue != root.secondsIntervalValueDefault
                        }
                    }
                }
            }
            Kirigami.Heading {
                text: i18nd("plasma_wallpaper_org.kde.image","Folders")
                level: 2
            }
            GridLayout {
                columns: 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                columnSpacing: Kirigami.Units.largeSpacing
                QtControls2.ScrollView {
                    id: foldersScroll
                    Layout.fillHeight: true
                    Layout.preferredWidth: 0.35 * parent.width
                    Layout.maximumWidth: Kirigami.Units.gridUnit * 16
                    Component.onCompleted: foldersScroll.background.visible = true;
                    ListView {
                        id: slidePathsView
                        model: imageWallpaper.slidePaths
                        delegate: Kirigami.SwipeListItem {
                            width: slidePathsView.width
                            // content item includes its own padding
                            padding: 0
                            // Don't need a highlight or hover effects
                            hoverEnabled: false
                            contentItem: Kirigami.BasicListItem {
                                // Don't need a highlight or hover effects
                                hoverEnabled: false
                                separatorVisible: false

                                // Header: the folder
                                label: {
                                    var strippedPath = modelData.replace(/\/+$/, "");
                                    return strippedPath.split('/').pop()
                                }
                                // Subtitle: the path to the folder
                                subtitle: {
                                    var strippedPath = modelData.replace(/\/+$/, "");
                                    return strippedPath.replace(/\/[^\/]*$/, '');;
                                }

                                QtControls2.ToolTip.text: modelData
                                QtControls2.ToolTip.visible: hovered
                                QtControls2.ToolTip.delay: 1000
                                QtControls2.ToolTip.timeout: 5000
                            }

                            actions: [
                                Kirigami.Action {
                                    iconName: "list-remove"
                                    tooltip: i18nd("plasma_wallpaper_org.kde.image", "Remove Folder")
                                    onTriggered: imageWallpaper.removeSlidePath(modelData)
                                },
                                Kirigami.Action {
                                    icon.name: "document-open-folder"
                                    tooltip: i18nd("plasma_wallpaper_org.kde.image", "Open Folder")
                                    onTriggered: imageWallpaper.openFolder(modelData)
                                }
                            ]
                        }

                        Kirigami.PlaceholderMessage {
                            anchors.centerIn: parent
                            width: parent.width - (Kirigami.Units.largeSpacing * 4)
                            visible: slidePathsView.count === 0
                            text: i18nd("plasma_wallpaper_org.kde.image", "There are no wallpaper locations configured")
                        }
                    }
                }
                Loader {
                    sourceComponent: thumbnailsComponent
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    anchors.fill: undefined
                }
                QtControls2.Button {
                    Layout.alignment: Qt.AlignRight
                    icon.name: "list-add"
                    text: i18nd("plasma_wallpaper_org.kde.image","Add Folder...")
                    onClicked: imageWallpaper.showAddSlidePathsDialog()
                }
                NewStuff.Button {
                    Layout.alignment: Qt.AlignRight
                    configFile: "wallpaper.knsrc"
                    text: i18nd("plasma_wallpaper_org.kde.image", "Get New Wallpapers...")
                    viewMode: NewStuff.Page.ViewMode.Preview
                    onChangedEntriesChanged: imageWallpaper.newStuffFinished();
                }
            }
        }
    }

    Component {
        id: thumbnailsComponent

        Item {
            property var imageModel: (configDialog.currentWallpaper === "org.kde.image") ? imageWallpaper.wallpaperModel : imageWallpaper.slideFilterModel

            KCM.GridView {
                id: wallpapersGrid
                anchors.fill: parent

                function resetCurrentIndex() {
                    //that min is needed as the module will be populated in an async way
                    //and only on demand so we can't ensure it already exists
                    view.currentIndex = Qt.binding(function() { return Math.min(imageModel.indexOf(cfg_Image), imageModel.count - 1) });
                }

                //kill the space for label under thumbnails
                view.model: imageModel
                Component.onCompleted: {
                    imageModel.usedInConfig = true;
                    resetCurrentIndex()
                }
                view.delegate: WallpaperDelegate {
                    color: cfg_Color
                }

                Kirigami.PlaceholderMessage {
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.largeSpacing * 2
                    // FIXME: this is needed to vertically center it in the grid for some reason
                    anchors.topMargin: wallpapersGrid.height / 2
                    visible: wallpapersGrid.view.count === 0
                    text: i18nd("plasma_wallpaper_org.kde.image", "There are no wallpapers in this slideshow")
                }
            }

            KCM.SettingHighlighter {
                target: wallpapersGrid
                highlight: configDialog.currentWallpaper === "org.kde.image" && cfg_Image != cfg_ImageDefault
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
            sourceComponent: (configDialog.currentWallpaper == "org.kde.image") ? thumbnailsComponent :
                ((configDialog.currentWallpaper == "org.kde.slideshow") ? foldersComponent : undefined)
        }
    }

    RowLayout {
        id: buttonsRow
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        visible: configDialog.currentWallpaper == "org.kde.image"
        QtControls2.Button {
            icon.name: "list-add"
            text: i18nd("plasma_wallpaper_org.kde.image","Add Image...")
            onClicked: imageWallpaper.showFileDialog();
        }
        NewStuff.Button {
            Layout.alignment: Qt.AlignRight
            configFile: "wallpaper.knsrc"
            text: i18nd("plasma_wallpaper_org.kde.image", "Get New Wallpapers...")
            viewMode: NewStuff.Page.ViewMode.Preview
            onChangedEntriesChanged: imageWallpaper.newStuffFinished();
        }
    }
}
