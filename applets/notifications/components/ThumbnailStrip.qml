/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2024 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import org.kde.plasma.components as PlasmaComponents3
import org.kde.kirigami as Kirigami

import org.kde.kquickcontrolsaddons as KQCAddons

import org.kde.plasma.private.notifications as Notifications

import "../global"

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
        onActionTriggered: action => modelInterface.fileActionInvoked(action)
    }

    Notifications.Thumbnailer {
        id: thumbnailer

        readonly property real ratio: pixmapSize.height ? pixmapSize.width / pixmapSize.height : 1

        url: urls[0]
        // height is dynamic, so request a "square" size and then show it fitting to aspect ratio
        // Also use popupWidth instead of our width to ensure it is fixed and doesn't
        // change temporarily during (re)layouting
        size: Qt.size(Globals.popupWidth, Globals.popupWidth)
    }

    KQCAddons.QPixmapItem {
        id: previewBackground
        anchors {
            fill: parent
            leftMargin: -modelInterface.popupLeftPadding
            topMargin: -modelInterface.popupTopPadding
            rightMargin: -modelInterface.popupRightPadding
            bottomMargin: -modelInterface.popupBottomPadding
        }
        fillMode: Image.PreserveAspectCrop
        layer.enabled: true
        opacity: 0.25
        pixmap: thumbnailer.pixmap
        layer.effect: FastBlur {
            source: previewBackground
            anchors.fill: previewBackground
            radius: 30
        }
    }

    DraggableFileArea {
        id: dragArea
        anchors.fill: previewBackground
        dragParent: previewIcon
        dragPixmapSize: previewIcon.height
        dragPixmap: thumbnailer.hasPreview ? thumbnailer.pixmap : thumbnailer.iconName
        dragUrl: thumbnailer.url

        onActivated: modelInterface.openUrl(thumbnailer.url)
        onContextMenuRequested: (pos) => {
            // avoid menu button glowing if we didn't actually press it
            menuButton.checked = false;

            fileMenu.visualParent = this;
            fileMenu.open(pos.x, pos.y);
        }
    }

    KQCAddons.QPixmapItem {
        id: previewPixmap
        anchors {
            fill: previewBackground
            margins: Kirigami.Units.smallSpacing
        }
        pixmap: thumbnailer.pixmap
        smooth: true
        fillMode: Image.PreserveAspectFit

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
                margins: Kirigami.Units.smallSpacing
            }
            spacing: Kirigami.Units.smallSpacing

            Item {
                Layout.fillWidth: true
            }
            ActionContainer {
                id: actionContainer
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
