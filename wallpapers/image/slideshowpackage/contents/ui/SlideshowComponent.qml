/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15

import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami
import org.kde.plasma.wallpapers.image 2.0 as PlasmaWallpaper

/**
 * For proper alignment, an ancestor **MUST** have id "appearanceRoot" and property "parentLayout"
 */
ColumnLayout {
    id: slideshowComponent
    property var configuration: wallpaper.configuration
    property var screenSize: Qt.size(Screen.width, Screen.height)

    property int hoursIntervalValue: Math.floor(cfg_SlideInterval / 3600)
    property int minutesIntervalValue: Math.floor(cfg_SlideInterval % 3600) / 60
    property int secondsIntervalValue: cfg_SlideInterval % 3600 % 60

    property int hoursIntervalValueDefault: Math.floor(cfg_SlideIntervalDefault / 3600)
    property int minutesIntervalValueDefault: Math.floor(cfg_SlideIntervalDefault % 3600) / 60
    property int secondsIntervalValueDefault: cfg_SlideIntervalDefault % 3600 % 60

    onHoursIntervalValueChanged: hoursInterval.value = hoursIntervalValue
    onMinutesIntervalValueChanged: minutesInterval.value = minutesIntervalValue
    onSecondsIntervalValueChanged: secondsInterval.value = secondsIntervalValue

    spacing: 0

    Kirigami.FormLayout {
        id: form

        Layout.bottomMargin: Kirigami.Units.largeSpacing

        Component.onCompleted: function () {
            if (typeof appearanceRoot !== "undefined") {
                twinFormLayouts = appearanceRoot.parentLayout;
            }
        }

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
                        if (model[i]["slideshowMode"] === configuration.SlideshowMode) {
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

    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true

        spacing: 0

        ColumnLayout {
            spacing: 0
            Layout.fillHeight: true
            Layout.preferredWidth: 0.35 * parent.width
            Layout.maximumWidth: Kirigami.Units.gridUnit * 16

            Kirigami.Separator {
                Layout.fillWidth: true
            }
            QQC2.ScrollView {
                id: foldersScroll
                Layout.fillWidth: true
                Layout.fillHeight: true

                background: Rectangle {
                    Kirigami.Theme.inherit: false
                    Kirigami.Theme.colorSet: Kirigami.Theme.View
                    color: Kirigami.Theme.backgroundColor
                }

                ListView {
                    id: slidePathsView
                    headerPositioning: ListView.OverlayHeader
                    header: Kirigami.InlineViewHeader {
                        width: slidePathsView.width
                        text: i18nd("plasma_wallpaper_org.kde.image", "Folders")
                        actions: [
                            Kirigami.Action {
                                icon.name: "folder-add-symbolic"
                                text: i18ndc("plasma_wallpaper_org.kde.image", "@action button the thing being added is a folder", "Add…")
                                onTriggered: root.openChooserDialog()
                            }
                        ]
                    }
                    model: imageWallpaper.slidePaths
                    delegate: Kirigami.SubtitleDelegate {
                        id: baseListItem

                        required property var modelData

                        width: slidePathsView.width
                        // Don't need a highlight or hover effects
                        hoverEnabled: false
                        down: false

                        text: {
                            var strippedPath = baseListItem.modelData.replace(/\/+$/, "");
                            return strippedPath.split('/').pop()
                        }
                        // Subtitle: the path to the folder
                        subtitle: {
                            var strippedPath = baseListItem.modelData.replace(/\/+$/, "");
                            return strippedPath.replace(/\/[^\/]*$/, '');;
                        }

                        contentItem: RowLayout {
                            spacing: Kirigami.Units.smallSpacing

                            Kirigami.TitleSubtitle {
                                Layout.fillWidth: true
                                // Header: the folder
                                title: baseListItem.text
                                subtitle: baseListItem.subtitle
                            }

                            QQC2.ToolButton {
                                icon.name: "edit-delete-remove-symbolic"
                                text: i18nd("plasma_wallpaper_org.kde.image", "Remove Folder")
                                display: QQC2.Button.IconOnly
                                onClicked: imageWallpaper.removeSlidePath(baseListItem.modelData)

                                QQC2.ToolTip.visible: hovered
                                QQC2.ToolTip.text: text
                                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                            }

                            QQC2.ToolButton {
                                icon.name: "document-open-folder"
                                text: i18nd("plasma_wallpaper_org.kde.image", "Open Folder…")
                                display: QQC2.Button.IconOnly
                                onClicked: Qt.openUrlExternally(baseListItem.modelData)

                                QQC2.ToolTip.visible: hovered
                                QQC2.ToolTip.text: text
                                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                            }
                        }
                    }

                    Kirigami.PlaceholderMessage {
                        anchors.centerIn: parent
                        width: parent.width - (Kirigami.Units.largeSpacing * 4)
                        visible: slidePathsView.count === 0
                        text: i18nd("plasma_wallpaper_org.kde.image", "There are no wallpaper locations configured")
                    }
                }
            }
        }

        Kirigami.Separator {
            Layout.fillHeight: true
        }

        Loader {
            Layout.fillWidth: true
            Layout.fillHeight: true
            anchors.fill: undefined

            Component.onCompleted: () => {
                this.setSource("ThumbnailsComponent.qml", {"screenSize": slideshowComponent.screenSize});
            }
        }
    }
}
