/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick 2.15

import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kirigami 2.20 as Kirigami
import org.kde.plasma.private.clipboard 0.1 as Private // image provider

ClipboardItemDelegate {
    id: menuItem
    Accessible.name: menuItem.model?.display ?? ""
    mainItem: Item {
        id: previewItem
        implicitHeight: Kirigami.Units.gridUnit * 4 + Kirigami.Units.smallSpacing * 2

        Drag.active: menuItem.dragHandler.active
        Drag.dragType: Drag.Automatic
        Drag.supportedActions: Qt.CopyAction
        Drag.mimeData: {
            "text/uri-list": menuItem.model?.display.split(" ") ?? [],
        }

        ListView {
            id: previewList
            model: menuItem.model?.display.split(" ", maximumNumberOfPreviews) ?? 0
            property int itemWidth: Kirigami.Units.gridUnit * 4
            property int itemHeight: Kirigami.Units.gridUnit * 4
            interactive: false

            spacing: Kirigami.Units.smallSpacing
            orientation: Qt.Horizontal
            width: (itemWidth + spacing) * model.length
            anchors {
                top: parent.top
                left: parent.left
                bottom: parent.bottom
            }

            delegate: Item {
                width: previewList.itemWidth
                height: previewList.itemHeight
                y: Math.round((parent.height - previewList.itemHeight) / 2)
                required property string modelData
                clip: true

                Image {
                    id: previewPixmap
                    anchors.centerIn: parent
                    asynchronous: true
                    sourceSize: Qt.size(previewList.itemWidth * 2, previewList.itemHeight * 2)
                    source: `image://klipperpreview/${modelData}`
                }
                Rectangle {
                    id: overlay
                    color: Kirigami.Theme.textColor
                    opacity: 0.6
                    height: Kirigami.Units.gridUnit
                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                    }
                }
                PlasmaComponents3.Label {
                    font: Kirigami.Theme.smallFont
                    color: Kirigami.Theme.backgroundColor
                    maximumLineCount: 1
                    anchors {
                        verticalCenter: overlay.verticalCenter
                        left: overlay.left
                        right: overlay.right
                        leftMargin: Kirigami.Units.smallSpacing
                        rightMargin: Kirigami.Units.smallSpacing
                    }
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                    text: {
                        let u = modelData.split("/");
                        return decodeURIComponent(u[u.length - 1]);
                    }
                    textFormat: Text.PlainText
                    Accessible.ignored: true
                }
            }
        }
        PlasmaComponents3.Label {
            property int additionalItems: menuItem.model?.display.split(" ").length ?? 0 - maximumNumberOfPreviews
            visible: additionalItems > 0
            opacity: 0.6
            text: i18ndc("klipper", "Indicator that there are more urls in the clipboard than previews shown", "+%1", additionalItems)
            textFormat: Text.PlainText
            anchors {
                left: previewList.right
                right: parent.right
                bottom: parent.bottom
                margins: Kirigami.Units.smallSpacing

            }
            verticalAlignment: Text.AlignBottom
            horizontalAlignment: Text.AlignCenter
            font: Kirigami.Theme.smallFont
        }
    }
}
