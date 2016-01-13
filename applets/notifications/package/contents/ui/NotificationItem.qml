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

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kquickcontrolsaddons 2.0

Item {
    id: notificationItem
    width: parent.width
    implicitHeight: Math.max(appIconItem.visible || imageItem.visible ? units.iconSizes.large : 0, mainLayout.height)

    // We need to clip here because we support displaying images through <img/>
    // and if we don't clip, they will be painted over the borders of the dialog/item
    clip: true

    signal close
    signal configure
    signal action(string actionId)

    property alias textItem: textItemLoader.sourceComponent
    property bool compact: false

    property alias icon: appIconItem.source
    property alias image: imageItem.image
    property alias summary: summaryLabel.text
    property alias configurable: settingsButton.visible
    property var created

    property ListModel actions: ListModel { }

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
            leftMargin: units.smallSpacing * 2
        }

        spacing: units.smallSpacing

        RowLayout {
            id: titleBar
            spacing: units.smallSpacing
            height: units.iconSizes.smallMedium

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

            // Force the whole thing to collapse if the children are invisible
            // If there is a big notification followed by a small one, the height
            // of the popup does not always shrink back, so this forces it to
            // height=0 when those are invisible. -1 means "default to implicitHeight"
            Layout.maximumHeight: textItemLoader.visible || actionsColumn.visible ? -1 : 0

            Loader {
                id: textItemLoader
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: textItemLoader.item && textItemLoader.item.text !== ""
                anchors {
                    leftMargin: units.smallSpacing * 2
                    rightMargin: units.smallSpacing * 2
                }
            }


            ColumnLayout {
                id: actionsColumn
                Layout.alignment: Qt.AlignTop
                Layout.maximumWidth: theme.mSize(theme.defaultFont).width * (compact ? 8 : 12)

                spacing: units.smallSpacing
                visible: notificationItem.actions && notificationItem.actions.count > 0

                Repeater {
                    id: actionRepeater
                    model: notificationItem.actions

                    PlasmaComponents.Button {
                        Layout.maximumWidth: actionsColumn.Layout.maximumWidth
                        text: model.text
                        onClicked: notificationItem.action(model.id)
                    }
                }
            }
        }
    }

}
