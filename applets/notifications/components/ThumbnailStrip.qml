/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2024 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import org.kde.plasma.components as PlasmaComponents3
import org.kde.kirigami as Kirigami

import org.kde.kquickcontrolsaddons as KQCAddons

import plasma.applet.org.kde.plasma.notifications as Notifications

Item {
    id: thumbnailArea

    property ModelInterface modelInterface

    // The protocol supports multiple URLs but so far it's only used to show
    // a single preview image, so this code is simplified a lot to accommodate
    // this usecase and drops everything else (fallback to app icon or ListView
    // for multiple files)
    property var urls: modelInterface.urls

    readonly property alias menuOpen: fileMenu.visible
    readonly property alias dragging: dragArea.dragging

    // Fix for BUG:462399
    implicitHeight: Kirigami.Units.iconSizes.enormous

    Notifications.FileMenu {
        id: fileMenu
        url: thumbnailer.url
        visualParent: menuButton
        onActionTriggered: action => thumbnailArea.modelInterface.fileActionInvoked(action)
    }

    Notifications.Thumbnailer {
        id: thumbnailer
        url: thumbnailArea.urls[0]
        // height is dynamic, so request a "square" size and then show it fitting to aspect ratio
        // Also use popupWidth instead of our width to ensure it is fixed and doesn't
        // change temporarily during (re)layouting
        size: Qt.size(Notifications.Globals.popupWidth, Notifications.Globals.popupWidth)
    }

    FastBlur {
        id: previewBackground
        anchors {
            fill: parent
            leftMargin: -thumbnailArea.modelInterface.popupLeftPadding
            topMargin: -thumbnailArea.modelInterface.popupTopPadding
            rightMargin: -thumbnailArea.modelInterface.popupRightPadding
            bottomMargin: -thumbnailArea.modelInterface.popupBottomPadding
        }
        source: ShaderEffectSource {
            sourceItem: previewImage
        }
        radius: 30
        opacity: 0.25
    }

    DraggableFileArea {
        id: dragArea
        anchors.fill: previewBackground
        dragParent: previewIcon
        dragPixmapSize: previewIcon.height
        dragImage: thumbnailer.hasPreview ? thumbnailer.image : thumbnailer.iconName
        dragUrl: thumbnailer.url

        onActivated: thumbnailArea.modelInterface.openUrl(thumbnailer.url)
        onContextMenuRequested: (pos) => {
            // avoid menu button glowing if we didn't actually press it
            menuButton.checked = false;

            fileMenu.visualParent = this;
            fileMenu.open(pos.x, pos.y);
        }

        KQCAddons.QImageItem {
            id: previewImage
            anchors.centerIn: parent
            width: nativeHeight > 0 ? Math.round(height * (nativeWidth / nativeHeight)) : 0
            height: parent.height - 2 * Kirigami.Units.smallSpacing
            image: thumbnailer.image
            smooth: true
        }

        Kirigami.Icon {
            id: previewIcon
            anchors.centerIn: parent
            width: height
            height: Kirigami.Units.iconSizes.roundedIconSize(parent.height)
            active: dragArea.hovered
            source: !thumbnailer.busy && !thumbnailer.hasPreview ? thumbnailer.iconName : ""
        }

        PlasmaComponents3.BusyIndicator {
            anchors.centerIn: parent
            running: thumbnailer.busy
            visible: thumbnailer.busy
        }

        RowLayout {
            id: thumbnailActionRow
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                // The thumbnail has small spacing around it,
                // and the buttons should have small spacing to the thumbnail.
                margins: 2 * Kirigami.Units.smallSpacing
            }
            spacing: Kirigami.Units.smallSpacing

            ActionContainer {
                id: actionContainer
                Layout.alignment: Qt.AlignRight
                Layout.horizontalStretchFactor: 1000 // Compress layout...
                Layout.maximumWidth: thumbnailActionRow.width - menuButton.width - thumbnailActionRow.spacing
                modelInterface: thumbnailArea.modelInterface
            }

            PlasmaComponents3.Button {
                id: menuButton
                Layout.alignment: Qt.AlignTop
                Accessible.name: tooltip.text
                icon.name: "application-menu"
                checkable: true

                onPressedChanged: {
                    if (pressed) {
                        // fake "pressed" while menu is open
                        checked = Qt.binding(function() {
                            return fileMenu.visible;
                        });

                        fileMenu.visualParent = this;
                        // -1 tells it to "align bottom left of visualParent (this)"
                        fileMenu.open(-1, -1);
                    }
                }

                PlasmaComponents3.ToolTip {
                    id: tooltip
                    text: i18nd("plasma_applet_org.kde.plasma.notifications", "More Options…")
                }
            }
        }
    }
}
