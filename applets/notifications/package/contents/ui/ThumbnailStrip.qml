/*
 *   Copyright 2016 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtGraphicalEffects 1.0

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.kquickcontrolsaddons 2.0 as KQCAddons

import org.kde.plasma.private.notifications 2.0 as Notifications

MouseArea {
    id: thumbnailArea

    // The protocol supports multiple URLs but so far it's only used to show
    // a single preview image, so this code is simplified a lot to accomodate
    // this usecase and drops everything else (fallback to app icon or ListView
    // for multiple files)
    property var urls

    readonly property bool dragging: plasmoid.nativeInterface.dragActive
    readonly property alias menuOpen: fileMenu.visible

    property int _pressX: -1
    property int _pressY: -1

    property int leftPadding: 0
    property int rightPadding: 0
    property int topPadding: 0
    property int bottomPadding: 0

    signal openUrl(string url)
    signal fileActionInvoked

    implicitHeight: Math.max(menuButton.height + 2 * menuButton.anchors.topMargin,
                             Math.round(Math.min(width / 3, width / thumbnailer.ratio)))
                    + topPadding + bottomPadding

    preventStealing: true
    cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
    acceptedButtons: Qt.LeftButton | Qt.RightButton

    onClicked: {
        if (mouse.button === Qt.LeftButton) {
            thumbnailArea.openUrl(thumbnailer.url)
        }
    }

    onPressed: {
        if (mouse.button === Qt.LeftButton) {
            _pressX = mouse.x;
            _pressY = mouse.y;
        } else if (mouse.button === Qt.RightButton) {
            // avoid menu button glowing if we didn't actually press it
            menuButton.checked = false;

            fileMenu.visualParent = this;
            fileMenu.open(mouse.x, mouse.y);
        }
    }
    onPositionChanged: {
        if (_pressX !== -1 && _pressY !== -1 && plasmoid.nativeInterface.isDrag(_pressX, _pressY, mouse.x, mouse.y)) {
            plasmoid.nativeInterface.startDrag(previewPixmap, thumbnailer.url, thumbnailer.pixmap);
            _pressX = -1;
            _pressY = -1;
        }
    }
    onReleased: {
        _pressX = -1;
        _pressY = -1;
    }
    onContainsMouseChanged: {
        if (!containsMouse) {
            _pressX = -1;
            _pressY = -1;
        }
    }

    Notifications.FileMenu {
        id: fileMenu
        url: thumbnailer.url
        visualParent: menuButton
        onActionTriggered: thumbnailArea.fileActionInvoked()
    }

    Notifications.Thumbnailer {
        id: thumbnailer

        readonly property real ratio: pixmapSize.height ? pixmapSize.width / pixmapSize.height : 1

        url: urls[0]
        // height is dynamic, so request a "square" size and then show it fitting to aspect ratio
        size: Qt.size(thumbnailArea.width, thumbnailArea.width)
    }

    KQCAddons.QPixmapItem {
        id: previewBackground
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        layer.enabled: true
        opacity: 0.25
        pixmap: thumbnailer.pixmap
        layer.effect: FastBlur {
            source: previewBackground
            anchors.fill: parent
            radius: 30
        }
    }

    Item {
        anchors {
            fill: parent
            leftMargin: thumbnailArea.leftPadding
            rightMargin: thumbnailArea.rightPadding
            topMargin: thumbnailArea.topPadding
            bottomMargin: thumbnailArea.bottomPadding
        }

        KQCAddons.QPixmapItem {
            id: previewPixmap
            anchors.fill: parent
            pixmap: thumbnailer.pixmap
            smooth: true
            fillMode: Image.PreserveAspectFit
        }

        PlasmaCore.IconItem {
            anchors.centerIn: parent
            width: height
            height: units.roundToIconSize(parent.height)
            usesPlasmaTheme: false
            source: !thumbnailer.busy && !thumbnailer.hasPreview ? thumbnailer.iconName : ""
        }

        PlasmaComponents.BusyIndicator {
            anchors.centerIn: parent
            running: thumbnailer.busy
            visible: thumbnailer.busy
        }

        PlasmaComponents.Button {
            id: menuButton
            anchors {
                top: parent.top
                right: parent.right
                margins: units.smallSpacing
            }
            tooltip: i18nd("plasma_applet_org.kde.plasma.notifications", "More Options...")
            Accessible.name: tooltip
            iconName: "application-menu"
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
        }
    }
}
