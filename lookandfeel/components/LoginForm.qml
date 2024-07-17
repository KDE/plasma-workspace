/*
    SPDX-FileCopyrightText: 2016 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.extras as PlasmaExtras
import org.kde.kirigami as Kirigami

/**
 * The main set of controls for logging in.
 * The layouts should be something like this:
 ┌──────────────────────────┐
 │                          │
 │    username              │ For some reason, it was not possible to do this 
 │    ┌────────────────┐    │ with GridLayout even though it should have what
 │    └────────────────┘    │ it needs to do this. It looked roughly like this:
 │    ┌────────────┐ ┌─┐    │        ┌────────────────────────────────┐
 │    └────────────┘ └─┘    │        │            username            │
 │    password      button  │        │                password btn    │
 │                          │        └────────────────────────────────┘
 └──────────────────────────┘
 ┌──────────────────────────┐
 │   username/password      │ There are times when the prompt should only show
 │    ┌────────────┐ ┌─┐    │ the username field or only the password field.
 │    └────────────┘ └─┘    │ We want the button to always be to the right of
 │                  button  │ the last visible field.
 └──────────────────────────┘
 ┌──────────────────────────┐
 │                          │ When there are no fields, show a labeled button.
 │          ┌────┐          │
 │          └────┘          │
 │        text button       │
 └──────────────────────────┘
 */
FocusScope {
    id: root
    property alias usernameField: usernameField
    property alias passwordField: passwordField
    property alias loginButton: loginButton
    property real spacing: Kirigami.Units.largeSpacing
    readonly property real preferredItemHeight: Math.max(usernameField.implicitHeight, passwordField.implicitHeight, loginButton.implicitHeight)
    signal loginRequested()

    baselineOffset: loginButton.y + loginButton.baselineOffset
    // We don't really care if the implicit size is larger than the true
    // implicit size for the current layout. Just make sure it's large enough.
    implicitWidth: Math.max(usernameField.implicitWidth,
                            passwordField.implicitWidth)
                            + root.spacing + preferredItemHeight
    implicitHeight: preferredItemHeight + root.spacing + preferredItemHeight

    PlasmaComponents3.TextField {
        id: usernameField
        width: parent.width
            - (passwordField.visible ? 0 : root.spacing + loginButton.width)
        height: root.preferredItemHeight
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: passwordField.visible ? -(root.spacing + height) / 2 : 0
        //if there's a username prompt it gets focus first, otherwise password does
        focus: visible && length === 0
        placeholderText: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Username")

        onAccepted: if (passwordField.visible) {
            passwordField.forceActiveFocus(Qt.TabFocusReason)
        } else {
            root.loginRequested()
        }
        onVisibleChanged: {
            if (!visible) {
                clear()
            }
        }
    }

    PlasmaExtras.PasswordField {
        id: passwordField
        width: parent.width - root.spacing - loginButton.width
        height: root.preferredItemHeight
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: usernameField.visible ? (root.spacing + height) / 2 : 0

        placeholderText: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Password")
        focus: !usernameField.visible || usernameField.length > 0

        onAccepted: root.loginRequested()
    }

    PlasmaComponents3.Button {
        id: loginButton
        baselineOffset: height
        width: display !== PlasmaComponents3.Button.IconOnly ?
            Math.min(parent.width, implicitWidth) : height
        height: Math.min(parent.height, root.preferredItemHeight)
        x: if (passwordField.visible || usernameField.visible) {
            return mirrored ? 0 : parent.width - width
        } else {
            return Math.round((parent.width - width) / 2)
        }
        anchors.verticalCenter: if (passwordField.visible) {
            return passwordField.verticalCenter
        } else if (usernameField.visible) {
            return usernameField.verticalCenter
        } else {
            return parent.verticalCenter
        }

        text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Log In")
        icon.name: mirrored ? "go-previous" : "go-next"
        display: usernameField.visible || passwordField.visible ?
            PlasmaComponents3.Button.IconOnly : PlasmaComponents3.Button.TextOnly
        onClicked: root.loginRequested()
        Keys.onEnterPressed: clicked()
        Keys.onReturnPressed: clicked()
    }
}
