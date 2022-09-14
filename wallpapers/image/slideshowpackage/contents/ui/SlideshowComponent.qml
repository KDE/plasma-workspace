/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15

import org.kde.newstuff 1.91 as NewStuff
import org.kde.kcm 1.5 as KCM
import org.kde.kirigami 2.12 as Kirigami
import org.kde.plasma.wallpapers.image 2.0 as PlasmaWallpaper

ColumnLayout {
    id: slideshowComponent

    property int hoursIntervalValue: Math.floor(cfg_SlideInterval / 3600)
    property int minutesIntervalValue: Math.floor(cfg_SlideInterval % 3600) / 60
    property int secondsIntervalValue: cfg_SlideInterval % 3600 % 60

    property int hoursIntervalValueDefault: Math.floor(cfg_SlideIntervalDefault / 3600)
    property int minutesIntervalValueDefault: Math.floor(cfg_SlideIntervalDefault % 3600) / 60
    property int secondsIntervalValueDefault: cfg_SlideIntervalDefault % 3600 % 60

    onHoursIntervalValueChanged: hoursInterval.value = hoursIntervalValue
    onMinutesIntervalValueChanged: minutesInterval.value = minutesIntervalValue
    onSecondsIntervalValueChanged: secondsInterval.value = secondsIntervalValue

    Kirigami.FormLayout {
        twinFormLayouts: parentLayout

        RowLayout {
            id: slideshowModeRow
            Kirigami.FormData.label: i18nd("plasma_wallpaper_org.kde.image", "Order:")

            QQC2.ComboBox {
                id: slideshowModeComboBox

                model: [
                    {
                        'label': i18nd("plasma_wallpaper_org.kde.image", "Random"),
                        'slideshowMode':  PlasmaWallpaper.SortingMode.Random
                    },
                    {
                        'label': i18nd("plasma_wallpaper_org.kde.image", "A to Z"),
                        'slideshowMode':  PlasmaWallpaper.SortingMode.Alphabetical
                    },
                    {
                        'label': i18nd("plasma_wallpaper_org.kde.image", "Z to A"),
                        'slideshowMode':  PlasmaWallpaper.SortingMode.AlphabeticalReversed
                    },
                    {
                        'label': i18nd("plasma_wallpaper_org.kde.image", "Date modified (newest first)"),
                        'slideshowMode':  PlasmaWallpaper.SortingMode.ModifiedReversed
                    },
                    {
                        'label': i18nd("plasma_wallpaper_org.kde.image", "Date modified (oldest first)"),
                        'slideshowMode':  PlasmaWallpaper.SortingMode.Modified
                    }
                ]
                textRole: "label"
                onActivated: {
                    cfg_SlideshowMode = model[currentIndex]["slideshowMode"];
                }
                Component.onCompleted: setMethod();
                function setMethod() {
                    for (var i = 0; i < model.length; i++) {
                        if (model[i]["slideshowMode"] === wallpaper.configuration.SlideshowMode) {
                            slideshowModeComboBox.currentIndex = i;
                            break;
                        }
                    }
                }

                KCM.SettingHighlighter {
                    highlight: cfg_SlideshowMode != cfg_SlideshowModeDefault
                }
            }

            QQC2.CheckBox {
                id: slideshowFoldersFirstCheckBox
                text: i18nd("plasma_wallpaper_org.kde.image", "Group by folders")
                checked: root.cfg_SlideshowFoldersFirst
                onToggled: cfg_SlideshowFoldersFirst = slideshowFoldersFirstCheckBox.checked

                KCM.SettingHighlighter {
                    highlight: root.cfg_SlideshowFoldersFirst !== cfg_SlideshowFoldersFirstDefault
                }
            }
        }

        // FIXME: there should be only one spinbox: QtControls spinboxes are still too limited for it tough
        RowLayout {
            Kirigami.FormData.label: i18nd("plasma_wallpaper_org.kde.image", "Change every:")
            QQC2.SpinBox {
                id: hoursInterval
                value: slideshowComponent.hoursIntervalValue
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
                    highlight: slideshowComponent.hoursIntervalValue != slideshowComponent.hoursIntervalValueDefault
                }
            }

            QQC2.SpinBox {
                id: minutesInterval
                value: slideshowComponent.minutesIntervalValue
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
                    highlight: slideshowComponent.minutesIntervalValue != slideshowComponent.minutesIntervalValueDefault
                }
            }

            QQC2.SpinBox {
                id: secondsInterval
                value: slideshowComponent.secondsIntervalValue
                from: slideshowComponent.hoursIntervalValue === 0 && slideshowComponent.minutesIntervalValue === 0 ? 1 : 0
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
                    highlight: slideshowComponent.secondsIntervalValue != slideshowComponent.secondsIntervalValueDefault
                }
            }
        }
    }

    Kirigami.Heading {
        text: i18nd("plasma_wallpaper_org.kde.image", "Folders")
        level: 2
    }

    GridLayout {
        columns: 2
        Layout.fillWidth: true
        Layout.fillHeight: true
        columnSpacing: Kirigami.Units.largeSpacing

        QQC2.ScrollView {
            id: foldersScroll
            Layout.fillHeight: true
            Layout.preferredWidth: 0.35 * parent.width
            Layout.maximumWidth: Kirigami.Units.gridUnit * 16
            Component.onCompleted: foldersScroll.background.visible = true;

            // HACK: workaround for https://bugreports.qt.io/browse/QTBUG-83890
            QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff

            ListView {
                id: slidePathsView
                model: imageWallpaper.slidePaths
                delegate: Kirigami.SwipeListItem {
                    id: baseListItem
                    width: slidePathsView.width
                    // content item includes its own padding
                    padding: 0
                    // Don't need a highlight or hover effects
                    hoverEnabled: false
                    contentItem: Kirigami.BasicListItem {
                        width: slidePathsView.width - baseListItem.overlayWidth
                        // Don't want a background highlight effect, but we can't just
                        // set hoverEnabled to false, since then the tooltip will
                        // never appear!
                        activeBackgroundColor: "transparent"
                        activeTextColor: Kirigami.Theme.textColor
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

                        QQC2.ToolTip.text: modelData
                        QQC2.ToolTip.visible: hovered && (labelItem.truncated || subtitleItem.truncated)
                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
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
                            onTriggered: Qt.openUrlExternally(modelData)
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
            source: "ThumbnailsComponent.qml"
            Layout.fillWidth: true
            Layout.fillHeight: true
            anchors.fill: undefined
        }

        QQC2.Button {
            Layout.alignment: Qt.AlignRight
            icon.name: "list-add"
            text: i18nd("plasma_wallpaper_org.kde.image","Add Folder…")
            onClicked: imageWallpaper.showAddSlidePathsDialog()
        }

        NewStuff.Button {
            Layout.alignment: Qt.AlignRight
            configFile: Kirigami.Settings.isMobile ? "wallpaper-mobile.knsrc" : "wallpaper.knsrc"
            text: i18nd("plasma_wallpaper_org.kde.image", "Get New Wallpapers…")
            viewMode: NewStuff.Page.ViewMode.Preview
        }
    }
}
