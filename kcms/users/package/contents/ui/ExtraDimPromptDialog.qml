/*
    SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtGraphicalEffects 1.15 as GE
import org.kde.kirigami 2.20 as Kirigami

Kirigami.PromptDialog {
    id: dialog

    required property Item windowContentItem

    readonly property int overlayFadeDuration: Kirigami.Units.veryLongDuration
    readonly property int contentFadeDuration: Kirigami.Units.veryLongDuration
    readonly property int contentFadeDelay: Kirigami.Units.veryLongDuration

    enter: Transition {
        SequentialAnimation {
            PropertyAction { property: "opacity"; value: 0 }
            PauseAnimation { duration: dialog.overlayFadeDuration + dialog.contentFadeDelay }
            NumberAnimation { property: "opacity"; to: 1; easing.type: Easing.InOutQuad; duration: dialog.contentFadeDuration }
        }
    }

    exit: Transition {
        SequentialAnimation {
            NumberAnimation { property: "opacity"; to: 0; easing.type: Easing.InOutQuad; duration: dialog.contentFadeDuration }
            PauseAnimation { duration: dialog.overlayFadeDuration + dialog.contentFadeDelay }
        }
    }

    QQC2.Overlay.modal: Rectangle {
        id: overlay
        color: Qt.rgba(0, 0, 0, 0.3)

        // the opacity of the item is changed internally by QQuickPopup on open/close
        Behavior on opacity {
            id: behavior
            SequentialAnimation {
                // Let the dialog exit first
                PauseAnimation {
                    duration: (behavior.targetValue === 0) ? (dialog.contentFadeDuration + dialog.contentFadeDelay) : 0
                }
                NumberAnimation {
                    duration: dialog.overlayFadeDuration
                    easing.type: Easing.InOutQuad
                }
            }
        }

        GE.FastBlur {
            anchors.fill: parent
            source: dialog.windowContentItem
            radius: 64 * overlay.opacity
        }

        GE.RadialGradient {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.4; color: Qt.rgba(0,0,0,0) }
                GradientStop { position: 0.7; color: Qt.rgba(0,0,0,0.8) }
            }
        }
    }
}
