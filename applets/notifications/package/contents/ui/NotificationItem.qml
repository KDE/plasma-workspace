/*
 *   Copyright 2011 Marco Martin <notmart@gmail.com>
 *   Copyright 2014 Kai Uwe Broulik <kde@privat.broulik.de>
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
import QtQuick.Controls.Private 1.0

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kquickcontrolsaddons 2.0

MouseArea {
    id: notificationItem
    width: parent.width
    implicitHeight: Math.max(appIconItem.visible || imageItem.visible ? units.iconSizes.large : 0, mainLayout.height)

    // We need to clip here because we support displaying images through <img/>
    // and if we don't clip, they will be painted over the borders of the dialog/item
    clip: true

    signal close
    signal configure
    signal action(string actionId)
    signal openUrl(url url)

    property bool compact: false

    property alias icon: appIconItem.source
    property alias image: imageItem.image
    property alias summary: summaryLabel.text
    property alias body: bodyText.text
    property alias configurable: settingsButton.visible
    property var created
    property var urls: []

    property int maximumTextHeight: -1

    property ListModel actions: ListModel { }

    property bool hasDefaultAction: false

    readonly property bool dragging: thumbnailStripLoader.item ? thumbnailStripLoader.item.dragging : false

    onClicked: {
        // the MEL would close the notification before the action button
        // onClicked handler would fire effectively breaking notification actions
        if (pressedAction()) {
            return
        }

        if (hasDefaultAction) {
            // the notifications was clicked, trigger the default action if set
            action("default")
        }
    }

    function pressedAction() {
        for (var i = 0, count = actionRepeater.count; i < count; ++i) {
            var item = actionRepeater.itemAt(i)
            if (item.pressed) {
                return item
            }
        }

        if (thumbnailStripLoader.item) {
            var item = thumbnailStripLoader.item.pressedAction()
            if (item) {
                return item
            }
        }

        if (settingsButton.pressed) {
            return settingsButton
        }

        if (closeButton.pressed) {
            return closeButton
        }

        return null
    }

    function updateTimeLabel() {
        if (!created || created.getTime() <= 0) {
            timeLabel.text = ""
            return
        }
        var currentTime = new Date().getTime()
        var createdTime = created.getTime()
        var d = (currentTime - createdTime) / 1000
        if (d < 10) {
            timeLabel.text = i18nc("notification was just added, keep short", "Just now")
        } else if (d < 20) {
            timeLabel.text = i18nc("10 seconds ago, keep short", "10 s ago");
        } else if (d < 40) {
            timeLabel.text = i18nc("30 seconds ago, keep short", "30 s ago");
        } else if (d < 60 * 60) {
            timeLabel.text = i18ncp("minutes ago, keep short", "%1 min ago", "%1 min ago", Math.round(d / 60))
        } else if (d <= 60 * 60 * 23) {
            timeLabel.text = Qt.formatTime(created, Qt.locale().timeFormat(Locale.ShortFormat).replace(/.ss?/i, ""))
        } else {
            var yesterday = new Date()
            yesterday.setDate(yesterday.getDate() - 1) // this will wrap
            yesterday.setHours(0)
            yesterday.setMinutes(0)
            yesterday.setSeconds(0)

            if (createdTime > yesterday.getTime()) {
                timeLabel.text = i18nc("notification was added yesterday, keep short", "Yesterday");
            } else {
                timeLabel.text = i18ncp("notification was added n days ago, keep short",
                                        "%1 day ago", "%1 days ago",
                                        Math.round((currentTime - yesterday.getTime()) / 1000 / 3600 / 24));
            }
        }
    }

    Timer {
        interval: 15000
        running: plasmoid.expanded
        repeat: true
        triggeredOnStart: true
        onTriggered: updateTimeLabel()
    }

    PlasmaCore.IconItem {
        id: appIconItem

        width: units.iconSizes.large
        height: units.iconSizes.large

        anchors {
            top: parent.top
            left: parent.left
        }

        visible: !imageItem.visible && valid
        animated: false
    }

    QImageItem {
        id: imageItem
        anchors.fill: appIconItem

        smooth: true
        visible: nativeWidth > 0
    }

    ColumnLayout {
        id: mainLayout

        anchors {
            top: parent.top
            left: appIconItem.visible || imageItem.visible ? appIconItem.right : parent.left
            right: parent.right
            leftMargin: units.smallSpacing
        }

        spacing: Math.round(units.smallSpacing / 2)

        RowLayout {
            id: titleBar
            spacing: units.smallSpacing

            PlasmaExtras.Heading {
                id: summaryLabel
                Layout.fillWidth: true
                Layout.fillHeight: true
                verticalAlignment: Text.AlignVCenter
                level: 4
                elide: Text.ElideRight
                wrapMode: Text.NoWrap
            }

            PlasmaExtras.Heading {
                id: timeLabel
                Layout.fillHeight: true
                level: 5
                visible: text !== ""
                verticalAlignment: Text.AlignVCenter

                PlasmaCore.ToolTipArea {
                    anchors.fill: parent
                    subText: Qt.formatDateTime(created, Qt.DefaultLocaleLongDate)
                }
            }

            PlasmaComponents.ToolButton {
                id: settingsButton
                width: units.iconSizes.smallMedium
                height: width
                visible: false

                iconSource: "configure"

                onClicked: configure()
            }

            PlasmaComponents.ToolButton {
                id: closeButton

                width: units.iconSizes.smallMedium
                height: width
                flat: compact

                iconSource: "window-close"

                onClicked: close()
            }

        }

        RowLayout {
            id: bottomPart
            Layout.alignment: Qt.AlignTop
            spacing: units.smallSpacing

            // Force the whole thing to collapse if the children are invisible
            // If there is a big notification followed by a small one, the height
            // of the popup does not always shrink back, so this forces it to
            // height=0 when those are invisible. -1 means "default to implicitHeight"
            Layout.maximumHeight: bodyText.visible || actionsColumn.visible ? -1 : 0

            PlasmaExtras.ScrollArea {
                id: bodyTextScrollArea
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true

                implicitHeight: maximumTextHeight > 0 ? Math.min(maximumTextHeight, bodyText.paintedHeight) : bodyText.paintedHeight
                visible: bodyText.length > 0

                flickableItem.boundsBehavior: Flickable.StopAtBounds
                flickableItem.flickableDirection: Flickable.VerticalFlick
                horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff

                TextEdit {
                    id: bodyText
                    width: bodyTextScrollArea.width
                    enabled: !Settings.isMobile

                    color: PlasmaCore.ColorScope.textColor
                    selectedTextColor: theme.viewBackgroundColor
                    selectionColor: theme.viewFocusColor
                    font.capitalization: theme.defaultFont.capitalization
                    font.family: theme.defaultFont.family
                    font.italic: theme.defaultFont.italic
                    font.letterSpacing: theme.defaultFont.letterSpacing
                    font.pointSize: theme.defaultFont.pointSize
                    font.strikeout: theme.defaultFont.strikeout
                    font.underline: theme.defaultFont.underline
                    font.weight: theme.defaultFont.weight
                    font.wordSpacing: theme.defaultFont.wordSpacing
                    renderType: Text.NativeRendering
                    selectByMouse: true
                    readOnly: true
                    wrapMode: Text.Wrap
                    textFormat: TextEdit.RichText

                    onLinkActivated: Qt.openUrlExternally(link)

                    // ensure selecting text scrolls the view as needed...
                    onCursorRectangleChanged: {
                        var flick = bodyTextScrollArea.flickableItem
                        if (flick.contentY >= cursorRectangle.y) {
                            flick.contentY = cursorRectangle.y
                        } else if (flick.contentY + flick.height <= cursorRectangle.y + cursorRectangle.height) {
                            flick.contentY = cursorRectangle.y + cursorRectangle.height - flick.height
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.RightButton | Qt.LeftButton

                        onClicked: {
                            if (mouse.button == Qt.RightButton)
                                contextMenu.open(mouse.x, mouse.y)
                            else {
                                notificationItem.clicked(mouse)
                            }
                        }

                        PlasmaComponents.ContextMenu {
                            id: contextMenu
                            visualParent: parent

                            PlasmaComponents.MenuItem {
                                text: i18n("Copy")
                                onClicked: {
                                    bodyText.selectAll()
                                    bodyText.copy()
                                }
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                id: actionsColumn
                Layout.alignment: Qt.AlignTop
                Layout.maximumWidth: theme.mSize(theme.defaultFont).width * (compact ? 10 : 16)
                // this is so it never collapses but always follows what the Buttons below want
                // but also don't let the buttons get too narrow (e.g. "View" or "Open" button)
                Layout.minimumWidth: Math.max(units.gridUnit * 4, implicitWidth)

                spacing: units.smallSpacing
                visible: notificationItem.actions && notificationItem.actions.count > 0

                Repeater {
                    id: actionRepeater
                    model: notificationItem.actions

                    PlasmaComponents.Button {
                        Layout.fillWidth: true
                        Layout.preferredWidth: minimumWidth
                        Layout.maximumWidth: actionsColumn.Layout.maximumWidth
                        text: model.text
                        onClicked: notificationItem.action(model.id)
                    }
                }
            }
        }

        Loader {
            id: thumbnailStripLoader
            Layout.fillWidth: true
            Layout.preferredHeight: item ? item.implicitHeight : 0
            source: "ThumbnailStrip.qml"
            active: notificationItem.urls.length > 0
        }
    }
}
