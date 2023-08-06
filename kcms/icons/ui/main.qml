/*
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtCore
import QtQuick 2.7
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import QtQuick.Dialogs 6.3 as QtDialogs
import QtQuick.Controls 2.3 as QtControls
import org.kde.kirigami 2.14 as Kirigami
import org.kde.kquickcontrolsaddons 2.0 as KQCAddons
import org.kde.newstuff 1.91 as NewStuff
import org.kde.kcmutils as KCM


KCM.GridViewKCM {
    id: root

    view.model: kcm.iconsModel
    view.currentIndex: kcm.pluginIndex(kcm.iconsSettings.theme)
    enabled: !kcm.downloadingFile

    KCM.SettingStateBinding {
        configObject: kcm.iconsSettings
        settingName: "Theme"
    }

    DropArea {
        enabled: view.enabled
        anchors.fill: parent
        onEntered: {
            if (!drag.hasUrls) {
                drag.accepted = false;
            }
        }
        onDropped: kcm.installThemeFromFile(drop.urls[0])
    }

    actions: [
        Kirigami.Action {
            enabled: root.view.enabled
            text: i18n("Install from File…")
            icon.name: "document-import"
            onTriggered: fileDialogLoader.active = true
        },
        NewStuff.Action {
            text: i18n("Get New…")
            configFile: "icons.knsrc"
            onEntryEvent: function (entry, event) {
                if (event == NewStuff.Entry.StatusChangedEvent) {
                    kcm.ghnsEntriesChanged();
                } else if (event == NewStuff.Entry.AdoptedEvent) {
                    kcm.reloadConfig();
                }
            }
        }
    ]

    view.delegate: KCM.GridDelegate {
        id: delegate

        text: model.display
        toolTip: model.description

        thumbnailAvailable: typeof thumbFlow.previews === "undefined" || thumbFlow.previews.length > 0
        thumbnail: MouseArea {
            id: thumbArea

            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            hoverEnabled: true
            clip: thumbFlow.y < 0

            opacity: model.pendingDeletion ? 0.3 : 1
            Behavior on opacity {
                NumberAnimation { duration: Kirigami.Units.longDuration }
            }

            Timer {
                interval: 1000
                repeat: true
                running: thumbArea.containsMouse
                onRunningChanged: {
                    if (!running) {
                        thumbFlow.currentPage = 0;
                    }
                }
                onTriggered: {
                    if (!thumbFlow.allPreviesLoaded) {
                        thumbFlow.loadPreviews(-1 /*no limit*/);
                        thumbFlow.allPreviesLoaded = true;
                    }

                    ++thumbFlow.currentPage;
                    if (thumbFlow.currentPage >= thumbFlow.pageCount) {
                        stop();
                    }
                }
            }

            Flow {
                id: thumbFlow

                // undefined is "didn't load preview yet"
                // empty array is "no preview available"
                property var previews
                // initially we only load 6 and when the animation starts we'll load the rest
                property bool allPreviesLoaded: false

                property int currentPage
                readonly property int pageCount: Math.ceil(thumbRepeater.count / (thumbFlow.columns * thumbFlow.rows))

                readonly property int iconWidth: Math.floor(thumbArea.width / thumbFlow.columns)
                readonly property int iconHeight: Math.floor(thumbArea.height / thumbFlow.rows)

                readonly property int columns: 3
                readonly property int rows: 2

                function loadPreviews(limit) {
                    previews = kcm.previewIcons(model.themeName, Math.min(thumbFlow.iconWidth, thumbFlow.iconHeight), Screen.devicePixelRatio, limit);
                }

                width: parent.width
                y: -currentPage * iconHeight * rows

                Behavior on y {
                    NumberAnimation { duration: Kirigami.Units.longDuration }
                }

                Repeater {
                    id: thumbRepeater
                    model: thumbFlow.previews

                    Item {
                        width: thumbFlow.iconWidth
                        height: thumbFlow.iconHeight

                        KQCAddons.QPixmapItem {
                            anchors.centerIn: parent
                            width: Math.min(parent.width, nativeWidth)
                            height: Math.min(parent.height, nativeHeight)
                            // load on demand and avoid leaking a tiny corner of the icon
                            pixmap: thumbFlow.y < 0 || index < (thumbFlow.columns * thumbFlow.rows) ? modelData : undefined
                            smooth: true
                            fillMode: KQCAddons.QPixmapItem.PreserveAspectFit
                        }
                    }
                }

                Component.onCompleted: {
                    // avoid reloading it when icon sizes or dpr changes on startup
                    Qt.callLater(function() {
                        // We show 6 icons initially (3x2 grid), only load those
                        thumbFlow.loadPreviews(6 /*limit*/);
                    });
                }
            }
        }

        actions: [
            Kirigami.Action {
                icon.name: "edit-delete"
                tooltip: i18n("Remove Icon Theme")
                enabled: model.removable
                visible: !model.pendingDeletion
                onTriggered: model.pendingDeletion = true
            },
            Kirigami.Action {
                icon.name: "edit-undo"
                tooltip: i18n("Restore Icon Theme")
                visible: model.pendingDeletion
                onTriggered: model.pendingDeletion = false
            }
        ]
        onClicked: {
            if (!model.pendingDeletion) {
                kcm.iconsSettings.theme = model.themeName;
            }
            view.forceActiveFocus();
        }
        onDoubleClicked: {
            kcm.save();
        }
    }

    footer: ColumnLayout {
        Kirigami.InlineMessage {
            id: infoLabel
            Layout.fillWidth: true

            showCloseButton: true

            Connections {
                target: kcm
                function onShowSuccessMessage(message) {
                    infoLabel.type = Kirigami.MessageType.Positive;
                    infoLabel.text = message;
                    infoLabel.visible = true;
                }
                function onShowErrorMessage(message) {
                    infoLabel.type = Kirigami.MessageType.Error;
                    infoLabel.text = message;
                    infoLabel.visible = true;
                }
            }
        }

        RowLayout {
            id: progressRow
            visible: false

            QtControls.BusyIndicator {
                id: progressBusy
            }

            QtControls.Label {
                id: progressLabel
                Layout.fillWidth: true
                textFormat: Text.PlainText
                wrapMode: Text.WordWrap
            }

            Connections {
                target: kcm
                function onShowProgress() {
                    progressLabel.text = message;
                    progressBusy.running = true;
                    progressRow.visible = true;
                }
                function onHideProgress() {
                    progressBusy.running = false;
                    progressRow.visible = false;
                }
            }
        }
    }

    Loader {
        id: fileDialogLoader
        active: false
        sourceComponent: QtDialogs.FileDialog {
            title: i18n("Open Theme")
            currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
            nameFilters: [ i18n("Theme Files (*.tar.gz *.tar.bz2)") ]
            Component.onCompleted: open()
            onAccepted: {
                kcm.installThemeFromFile(selectedFile)
                fileDialogLoader.active = false
            }
            onRejected: {
                fileDialogLoader.active = false
            }
        }
    }
}
