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

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.kquickcontrolsaddons 2.0

import org.kde.plasma.private.notifications 1.0 as Notifications

ListView {
    id: previewList

    readonly property int itemSquareSize: units.gridUnit * 4

    // if it's only one file, show a larger preview
    // however if no preview could be generated, which we only know after we tried,
    // the delegate will use itemSquareSize instead as there's no point in showing a huge file icon
    readonly property int itemWidth: previewList.count === 1 ? width : itemSquareSize
    readonly property int itemHeight: previewList.count === 1 ? Math.round(width / 3) : itemSquareSize

    // by the time the "model" is populated, the Layout isn't finished yet, causing ListView to have a 0 width
    // hence it's based on the mainLayout.width instead
    readonly property int maximumItemCount: Math.floor(mainLayout.width / itemSquareSize)

    // whether we're currently dragging, this way we can keep the popup around during the entire
    // drag operation even if the mouse leaves the popup
    property bool dragging: Notifications.DragHelper.dragActive

    model: {
        var urls = notificationItem.urls
        if (urls.length <= maximumItemCount) {
            return urls
        }
        // if it wouldn't fit, remove one item in favor of the "+n" badge
        return urls.slice(0, maximumItemCount - 1)
    }
    orientation: ListView.Horizontal
    spacing: units.smallSpacing
    interactive: false

    footer: notificationItem.urls.length > maximumItemCount ? moreBadge : null

    function pressedAction() {
        for (var i = 0; i < count; ++i) {
            var item = itemAtIndex(i)
            if (item.pressed) {
                return item
            }
        }
    }

    // HACK ListView only provides itemAt(x,y) methods but since we don't scroll
    // we can make assumptions on what our layout looks like...
    function itemAtIndex(index) {
        return itemAt(index * (itemSquareSize + spacing), 0)
    }

    Component {
        id: moreBadge

        // if there's more urls than we can display, show a "+n" badge
        Item {
            width: moreLabel.width
            height: previewList.height

            PlasmaExtras.Heading {
                id: moreLabel
                anchors {
                    left: parent.left
                    // ListView doesn't add spacing before the footer
                    leftMargin: previewList.spacing
                    top: parent.top
                    bottom: parent.bottom
                }
                level: 3
                verticalAlignment: Text.AlignVCenter
                text: i18nc("Indicator that there are more urls in the notification than previews shown", "+%1", notificationItem.urls.length - previewList.count)
            }
        }
    }

    delegate: MouseArea {
        id: previewDelegate

        property int pressX: -1
        property int pressY: -1

        // clip is expensive, only clip if the QPixmapItem would leak outside
        clip: previewPixmap.height > height

        width: thumbnailer.hasPreview ? previewList.itemWidth : previewList.itemSquareSize
        height: thumbnailer.hasPreview ? previewList.itemHeight : previewList.itemSquareSize

        preventStealing: true
        cursorShape: Qt.OpenHandCursor
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onClicked: {
            if (mouse.button === Qt.LeftButton) {
                notificationItem.openUrl(modelData);
            }
        }

        onPressed: {
            if (mouse.button === Qt.LeftButton) {
                pressX = mouse.x;
                pressY = mouse.y;
            } else if (mouse.button === Qt.RightButton) {
                // avoid menu button glowing if we didn't actually press it
                menuButton.checked = false;

                thumbnailer.showContextMenu(mouse.x, mouse.y, modelData, this);
            }
        }
        onPositionChanged: {
            if (pressX !== -1 && pressY !== -1 && Notifications.DragHelper.isDrag(pressX, pressY, mouse.x, mouse.y)) {
                Notifications.DragHelper.startDrag(previewDelegate, modelData /*url*/, thumbnailer.pixmap);
                pressX = -1;
                pressY = -1;
            }
        }
        onReleased: {
            pressX = -1;
            pressY = -1;
        }
        onContainsMouseChanged: {
            if (!containsMouse) {
                pressX = -1;
                pressY = -1;
            }
        }

        // first item determins the ListView height
        Binding {
            target: previewList
            property: "implicitHeight"
            value: previewDelegate.height
            when: index === 0
        }

        Notifications.Thumbnailer {
            id: thumbnailer

            readonly property real ratio: pixmapSize.height ? pixmapSize.width / pixmapSize.height : 1

            url: modelData
            size: Qt.size(previewList.itemWidth, previewList.itemHeight)
        }

        QPixmapItem {
            id: previewPixmap

            anchors.centerIn: parent

            width: parent.width
            height: width / thumbnailer.ratio
            pixmap: thumbnailer.pixmap
            smooth: true
        }

        PlasmaCore.IconItem {
            anchors.fill: parent
            source: thumbnailer.iconName
            usesPlasmaTheme: false
        }

        Rectangle {
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            color: theme.textColor
            opacity: 0.6
            height: fileNameLabel.contentHeight

            PlasmaComponents.Label {
                id: fileNameLabel
                anchors {
                    fill: parent
                    leftMargin: units.smallSpacing
                    rightMargin: units.smallSpacing
                }
                wrapMode: Text.NoWrap
                height: implicitHeight // unset Label default height
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideMiddle
                font.pointSize: theme.smallestFont.pointSize
                color: theme.backgroundColor
                text: {
                    var splitUrl = modelData.split("/")
                    return splitUrl[splitUrl.length - 1]
                }
            }
        }

        PlasmaComponents.Button {
            id: menuButton
            anchors {
                top: parent.top
                right: parent.right
                margins: units.smallSpacing
            }
            width: Math.ceil(units.iconSizes.small + 2 * units.smallSpacing)
            height: width
            tooltip: i18n("More Options...")
            Accessible.name: tooltip
            checkable: true

            // -1 tells it to "align bottom left of item (this)"
            onClicked: {
                checked = Qt.binding(function() {
                    return thumbnailer.menuVisible;
                });

                thumbnailer.showContextMenu(-1, -1, modelData, this)
            }

            PlasmaCore.IconItem {
                anchors {
                    fill: parent
                    margins: units.smallSpacing
                }
                source: "application-menu"

                // From Plasma's ToolButtonStyle:
                active: parent.hovered
                colorGroup: parent.hovered ? PlasmaCore.Theme.ButtonColorGroup : PlasmaCore.ColorScope.colorGroup
            }
        }
    }
}
