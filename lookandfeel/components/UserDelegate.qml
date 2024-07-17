/*
    SPDX-FileCopyrightText: 2014 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Templates as T
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kirigami 2.20 as Kirigami

T.ItemDelegate {
    id: wrapper
    // If we're using software rendering, draw outlines instead of shadows
    // See https://bugs.kde.org/show_bug.cgi?id=398317
    readonly property bool softwareRendering: GraphicsInfo.api === GraphicsInfo.Software
    // We don't use required properties to automatically get model data because
    // some of these (e.g., the display name) need to be handled in a custom way
    property alias displayName: wrapper.text
    // Used externally to get the selected user's username
    property string userName: ""
    // The avatar path can be a plain path string or a url as a string.
    property string avatarPath: ""
    // This is its own property so we can set up a fallback icon in icon.name
    property string iconName: ""
    // Used externally to determine whether the selected user needs a password.
    property bool needsPassword: true
    // Whether to keep the text in the bounds of the contentItem
    property bool constrainText: true

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    baselineOffset: contentItem.y + contentItem.baselineOffset

    hoverEnabled: true
    highlighted: true

    padding: Kirigami.Units.largeSpacing
    bottomPadding: 0
    spacing: Kirigami.Units.largeSpacing * 2

    opacity: highlighted ? 1.0 : 0.5
    Behavior on opacity {
        OpacityAnimator {
            duration: Kirigami.Units.veryLongDuration
        }
    }

    icon.width: Kirigami.Units.iconSizes.enormous
    icon.height: Kirigami.Units.iconSizes.enormous
    // use a fallback icon when no icon name is given
    icon.name: iconName || "user-identity"
    // Make it bigger than other fonts to match the scale of the avatar better.
    // pixelSize is just pointSize/0.75. It does not reflect the real size.
    // We are using pointSize because it supports floats.
    font.pointSize: wrapper.icon.height / 6 * 0.75

    // ColumnLayout doesn't work with size/position animations
    contentItem: Column {
        spacing: parent.spacing
        baselineOffset: labelItem.y + label.baselineOffset
        ShaderEffect {
            id: shaderEffect
            readonly property Item source: ShaderEffectSource {
                sourceItem: faceIcon
                // software rendering is just a fallback so we can accept not having a rounded avatar here
                hideSource: !wrapper.softwareRendering
                live: true // otherwise the user in focus will show a blurred avatar
            }
            readonly property color colorBorder: Kirigami.Theme.textColor

            opacity: enabled ? 1.0 : 0.5
            Behavior on opacity {
                OpacityAnimator {
                    duration: Kirigami.Units.longDuration
                }
            }

            fragmentShader: "qrc:/qt/qml/org/kde/breeze/components/shaders/UserDelegate.frag.qsb"
            supportsAtlasTextures: true

            anchors.horizontalCenter: parent.horizontalCenter
            implicitWidth: wrapper.icon.width
            implicitHeight: wrapper.icon.height
            width: undefined
            height: undefined

            // Draw a translucent background circle under the user picture
            Rectangle {
                z: -1
                anchors.fill: parent
                anchors.margins: 1 // prevent fringing
                radius: width / 2
                color: Kirigami.Theme.backgroundColor
                opacity: 0.6
            }

            Kirigami.Icon {
                id: faceIcon
                anchors.centerIn: parent
                placeholder: wrapper.icon.name
                fallback: placeholder
                // Regardless of whether the source is a file path, local url,
                // remote url, image provider url or theme icon name,
                // Kirigami Icon should be able to get the image.
                // The fallback icon is dimmed when it's not set as the source,
                // so we use the fallback icon as the source when there is no avatar.
                source: wrapper.avatarPath || fallback
                // If no avatar/source status not ready, we use a theme icon.
                roundToIconSize: status !== Kirigami.Icon.Ready || source === fallback
                // When using a theme icon, keep it in the bounds of the circle.
                // We don't need to round because anchors.centerIn and roundToIconSize
                // already do that.
                width: roundToIconSize ? parent.width * Math.SQRT1_2 : parent.width
                height: roundToIconSize ? parent.height * Math.SQRT1_2 : parent.height
            }
        }

        Item {
            id: labelItem
            anchors.horizontalCenter: parent.horizontalCenter
            implicitWidth: shaderEffect.implicitWidth
            implicitHeight: label.implicitHeight
            PlasmaComponents3.Label {
                id: label
                anchors.horizontalCenter: parent.horizontalCenter
                width: wrapper.constrainText ?
                    shaderEffect.implicitWidth
                    : Window.width - wrapper.leftPadding - wrapper.rightPadding
                text: wrapper.text
                textFormat: Text.PlainText
                style: wrapper.softwareRendering ? Text.Outline : Text.Normal
                styleColor: Kirigami.Theme.backgroundColor // with no outline, doesn't matter
                wrapMode: Text.WordWrap
                maximumLineCount: wrapper.constrainText ? 3 : undefined
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignTop
                //make an indication that this has active focus, this only happens when reached with keyboard navigation
                font.underline: wrapper.activeFocus
            }
        }

        // We use states to keep animations in sync
        states: State {
            when: !wrapper.highlighted
            PropertyChanges {
                target: shaderEffect
                width: shaderEffect.implicitWidth - label.contentHeight / label.lineCount
                height: shaderEffect.implicitHeight - label.contentHeight / label.lineCount
            }
        }
        transitions: Transition {
            id: contentTransition
            enabled: false
            NumberAnimation {
                properties: "width,height"
                // Keep in sync with UserList animation durations
                duration: Kirigami.Units.veryLongDuration
                easing.type: Easing.InOutQuad
            }
        }
        // Enable transition after initialization
        Component.onCompleted: Qt.callLater(() => {contentTransition.enabled = true})
    }
}
