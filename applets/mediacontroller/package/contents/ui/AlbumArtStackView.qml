/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    id: container

    /**
     * Whether the album art image is available or in loading status
     */
    readonly property bool hasImage: (root.albumArt && albumArt.empty)
        || (albumArt.currentItem instanceof Image && (albumArt.currentItem.status === Image.Ready || albumArt.currentItem.status === Image.Loading))

    readonly property bool animating: exitTransition.running || popExitTransition.running

    /**
     * Whether the component is used in the compact representation
     */
    property bool inCompactRepresentation: false

    /**
     * Provides source item for \ShaderEffectSource
     */
    readonly property alias albumArt: albumArt

    onWidthChanged: geometryChangeTimer.restart();
    onHeightChanged: geometryChangeTimer.restart();

    function loadAlbumArt() {
        if (!root.albumArt) {
            albumArt.clear(StackView.PopTransition);
            return;
        }

        const oldImageRatio = albumArt.currentItem instanceof Image ? albumArt.currentItem.sourceSize.width / albumArt.currentItem.sourceSize.height : 1;
        const pendingImage = albumArtComponent.createObject(albumArt, {
            "source": root.albumArt,
            "sourceSize": Qt.size(container.width * Screen.devicePixelRatio, container.height * Screen.devicePixelRatio),
            "opacity": 0,
        });

        function replaceWhenLoaded() {
            if (pendingImage.status === Image.Loading) {
                return;
            }

            if (pendingImage.status === Image.Ready) {
                const newImageRatio = pendingImage.sourceSize.width / pendingImage.sourceSize.height;
                exitTransitionOpacityAnimator.duration = oldImageRatio === newImageRatio ? 0 : PlasmaCore.Units.longDuration;
            } else {
                // Load placeholder icon, but keep the invalid image to avoid flashing application icons
                exitTransitionOpacityAnimator.duration = 0;
            }

            albumArt.replace(pendingImage, {}, StackView.ReplaceTransition);
            pendingImage.statusChanged.disconnect(replaceWhenLoaded);
        }

        pendingImage.statusChanged.connect(replaceWhenLoaded);
        replaceWhenLoaded();
    }

    // Reload album art when size of container changes
    Timer {
        id: geometryChangeTimer
        interval: 250
        onTriggered: container.loadAlbumArt();
    }

    StackView {
        id: albumArt

        anchors.fill: parent

        readonly property string icon: (mpris2Source.currentData && mpris2Source.currentData["Desktop Icon Name"]) || "media-album-cover"

        replaceEnter: Transition {
            OpacityAnimator {
                from: 0
                to: 1
                duration: PlasmaCore.Units.longDuration
            }
        }

        replaceExit: Transition {
            id: exitTransition

            SequentialAnimation {
                PauseAnimation {
                    duration: PlasmaCore.Units.longDuration
                }

                /**
                * If the new ratio and the old ratio are different,
                * perform a fade-out animation for the old image
                * to prevent it from suddenly disappearing.
                */
                OpacityAnimator {
                    id: exitTransitionOpacityAnimator
                    from: 1
                    to: 0
                    duration: 0
                }
            }
        }

        popExit: Transition {
            id: popExitTransition

            OpacityAnimator {
                from: 1
                to: 0
                duration: PlasmaCore.Units.longDuration
            }
        }

        Component {
            id: albumArtComponent

            Image { // Album Art
                horizontalAlignment: Image.AlignRight
                verticalAlignment: Image.AlignVCenter
                fillMode: Image.PreserveAspectFit

                asynchronous: true
                cache: false

                StackView.onRemoved: {
                    source = ""; // HACK: Reduce memory usage
                    destroy();
                }
            }
        }

        Component {
            id: fallbackIconItem

            PlasmaCore.IconItem { // Fallback
                id: fallbackIcon

                anchors.margins: PlasmaCore.Units.largeSpacing * 2
                opacity: 0
                source: albumArt.icon

                NumberAnimation {
                    duration: PlasmaCore.Units.longDuration
                    easing.type: Easing.OutCubic
                    property: "opacity"
                    running: true
                    target: fallbackIcon
                    to: 1
                }
            }
        }

        // "No media playing" placeholder message
        Component {
            id: placeholderMessage

            // Put PlaceholderMessage in Item so PlaceholderMessage will not fill its parent.
            Item {
                property alias source: message.iconName

                PlasmaExtras.PlaceholderMessage {
                    id: message
                    anchors.centerIn: parent
                    width: parent.width // For text wrap
                    iconName: albumArt.icon
                    text: (root.isPlaying || root.state === "paused") ? i18n("No title") : i18n("No media playing")
                }
            }
        }

    }

    Loader {
        anchors.fill: parent
        visible: active

        active: (inCompactRepresentation || Plasmoid.expanded) && !container.hasImage
        asynchronous: true
        sourceComponent: root.track ? fallbackIconItem : placeholderMessage
    }
}

