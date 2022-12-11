/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2

import org.kde.kirigami 2.20 as Kirigami
import org.kde.plasma.private.fontview 0.1

Column {
    id: preview

    readonly property string fontFamily: sourceLoader.status === FontLoader.Ready ? sourceLoader.name : fontName
    readonly property bool atMax: fontSize === 128
    readonly property bool atMin: fontSize === 8

    property int face: 0
    property real fontSize: 14
    property string fontName
    property string styleName
    property string text

    spacing: 0

    Accessible.description: i18nc("@info:whatsthis", "This displays a preview of the selected font.")

    // For C++ side to open dialog
    signal requestChangePreviewText()

    function zoomIn() {
        fontSize = Math.min(128, fontSize + 4);
    }

    function zoomOut() {
        fontSize = Math.max(8, fontSize - 4);
    }

    FontPreviewBackend {
        id: backend
        name: preview.fontName
        face: preview.face
        previewCount: preview.width * preview.height / preview.fontSize / preview.fontSize / 1.2
    }

    FontLoader {
        id: sourceLoader
        source: preview.fontName
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
                text: i18nc("@item:inmenu", "Zoom In")
                shortcut: StandardKey.ZoomIn
                onTriggered: preview.zoomIn()
            }
        }

        QQC2.MenuItem {
            action: QQC2.Action {
                enabled: !preview.atMin
                icon.name: "zoom-out"
                text: i18nc("@item:inmenu", "Zoom Out")
                shortcut: StandardKey.ZoomOut
                onTriggered: preview.zoomOut()
            }
        }

        QQC2.MenuSeparator { }

        QQC2.Menu {
            title: i18nc("@item:inmenu", "Preview Type")

            Repeater {
                model: backend.unicodeRangeNames

                delegate: QQC2.MenuItem {
                    checkable: true
                    checked: backend.unicodeRangeIndex === index

                    text: modelData

                    onToggled: if (checked) {
                        backend.unicodeRangeIndex = index;
                    }
                }
            }
        }

        QQC2.MenuItem {
            action: QQC2.Action {
                enabled: backend.unicodeRangeIndex === 0
                icon.name: "edit-rename"
                text: i18nc("@item:inmenu", "Change Preview Text…")
                onTriggered: {
                    menu.close();
                    preview.requestChangePreviewText();
                }
            }
        }
    }

    Text {
        font.pointSize: 10
        text: preview.fontFamily.length > 0 ? `${preview.fontFamily} ${preview.styleName}` : i18nc("@label", "ERROR: Could not determine font's name.")
    }

    Kirigami.Separator {
        width: parent.width
    }

    Loader {
        width: parent.width
        asynchronous: true
        source: backend.unicodeRangeIndex === 0 ? "StandardFontPreview.qml" : "CharacterPreview.qml"
    }
}
