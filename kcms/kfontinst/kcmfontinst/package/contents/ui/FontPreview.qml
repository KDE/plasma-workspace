/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2

import org.kde.kirigami 2.20 as Kirigami
import org.kde.plasma.private.fontview 0.1

FontPreviewItem {
    id: preview

    readonly property bool atMax: pixelSize === 128
    readonly property bool atMin: pixelSize === 4

    Accessible.description: i18nc("@info:whatsthis", "This displays a preview of the selected font.")

    // For C++ side to open dialog
    signal requestChangePreviewText()

    function zoomIn() {
        pixelSize = Math.min(128, pixelSize + 4);
    }

    function zoomOut() {
        pixelSize = Math.max(4, pixelSize - 4);
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: menu.popup(preview, eventPoint.event.x, eventPoint.event.y);
    }

    WheelHandler {
        property int angleDelta: 0
        onWheel: {
            angleDelta += event.angleDelta.y;
            if (angleDelta >= 120) {
                angleDelta = 0;
                preview.zoomIn();
            } else if (angleDelta <= -120) {
                angleDelta = 0;
                preview.zoomOut();
            }
        }
    }

    // Preview widget pop-up menu
    QQC2.Menu {
        id: menu

        QQC2.MenuItem {
            action: QQC2.Action {
                enabled: !preview.atMax
                icon.name: "zoom-in"
                text: i18nc("@item:inmenu", "Zoom In")
                shortcut: StandardKey.ZoomIn
                onTriggered: waylandPreviewLoader.active ? waylandPreviewLoader.item.zoomIn() : preview.zoomIn()
            }
        }

        QQC2.MenuItem {
            action: QQC2.Action {
                enabled: !preview.atMin
                icon.name: "zoom-out"
                text: i18nc("@item:inmenu", "Zoom Out")
                shortcut: StandardKey.ZoomOut
                onTriggered: waylandPreviewLoader.active ? waylandPreviewLoader.item.zoomOut() : preview.zoomOut()
            }
        }

        QQC2.MenuSeparator { }

        QQC2.Menu {
            title: i18nc("@item:inmenu", "Preview Type")

            Repeater {
                model: preview.unicodeRangeNames

                delegate: QQC2.MenuItem {
                    checkable: true
                    checked: preview.unicodeRangeIndex === index

                    text: modelData

                    onToggled: if (checked) {
                        preview.unicodeRangeIndex = index;
                    }
                }
            }
        }

        QQC2.MenuItem {
            action: QQC2.Action {
                icon.name: "edit-rename"
                text: i18nc("@item:inmenu", "Change Preview Text…")
                onTriggered: {
                    menu.close();
                    preview.requestChangePreviewText();
                }
            }
        }
    }
}
