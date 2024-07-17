/*
    SPDX-FileCopyrightText: 2016 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.extras as PlasmaExtras
import org.kde.kirigami as Kirigami
import org.kde.plasma.private.keyboardindicator as KeyboardIndicator

T.Page {
    id: root
    /*
     * A list of Items (typically ActionButtons) to be shown in a Row beneath the prompts
     */
    property alias actionItems: actionItemsLayout.children

    /*
     * Whether to show or hide the list of action items as a whole.
     */
    property alias actionItemsVisible: actionItemsLayout.visible

    // The baseline offset position that should be ensured visible when the on screen keyboard is visible
    property real virtualKeyboardBoundary: baselineOffset + Kirigami.Units.smallSpacing

    // If you add a username input field, you should set this property so that
    // other components can use the username input field.
    property T.TextField usernameField: loginForm.usernameField

    // If you add a password input field, you should set this property so that
    // other components can use the password input field.
    property PlasmaExtras.PasswordField passwordField: loginForm.passwordField

    // If you add a login button, you should set this property so that
    // other components can use the login button.
    property T.AbstractButton loginButton: loginForm.loginButton

    // Whether we are waiting for a response from the login managing system.
    // Do not try to set this, it is managed by signals and slots/handlers.
    property bool waitingForResponse: false

    /*
     * A model with a list of users to show in the view.
     * There are different implementations in sddm greeter (UserModel) and
     * KScreenLocker (SessionsModel), so some roles will be missing.
     *
     * type: {
     *  name: string,
     *  realName: string,
     *  homeDir: string,
     *  icon: string,
     *  iconName?: string,
     *  needsPassword?: bool,
     *  displayNumber?: string,
     *  vtNumber?: int,
     *  session?: string
     *  isTty?: bool,
     * }
     */
    property alias userListModel: userListView.model

    property alias userListCurrentIndex: userListView.currentIndex
    property alias userListCurrentItem: userListView.currentItem
    property bool showUserList: true

    property alias userList: userListView

    // The default callback for requestLogin()
    property var requestLoginCallback

    /**
     * Use this function to initiate a login attempt.
     *
     * The callback should be a function that takes a username, password and
     * provides the logic necessary to actually login.
     * It should return a list of all signals that can be used to determine
     * whether we received a response so that logic to reset waitingForResponse
     * can be set up automatically.
     *
     * The reason why we don't use a login request signal and signal handler
     * for requesting a login like we did previously is that I want the order
     * in which code is executed to be clearer. With signal handlers,
     * it's more difficult to keep track of the exact order of events.
     */
    function requestLogin(username = usernameField?.visible ?
                          usernameField.text : userList.selectedUser,
                          password = passwordField?.text ?? "",
                          callback = requestLoginCallback)
    {
        if (!callback) {
            return;
        }
        // This is partly because it looks nicer, but more importantly it
        // works round a Qt bug that can trigger if the app is closed with a
        // TextField focused.
        //
        // See https://bugreports.qt.io/browse/QTBUG-55460
        // Also see commit cf87b9a1bf97e3ae53ce0fa31b4a399ea07fac21 in plasma-workspace.
        let focusedItem = Window.activeFocusItem;
        while (Window.activeFocusItem instanceof TextInput) {
            focusedItem = focusedItem.nextItemInFocusChain();
        }
        focusedItem?.forceActiveFocus(Qt.TabFocusReason);

        let responseSignalList = callback(username, password);
        root.waitingForResponse = responseSignalList.length > 0;
        if (!root.waitingForResponse) {
            return;
        }
        // set parent so that it won't be deleted prematurely
        let temp = slotComponent.createObject(root, {"slot": () => {
            root.waitingForResponse = false;
            temp.destroy();
        }})
        // The same object is having its slot connected to multiple signals.
        // The object will be deleted by the first signal that emits,
        // disconnecting from all of the signals in the process.
        for (let responseSignal of responseSignalList) {
            responseSignal.connect(temp.slot);
        }
        // This isn't a particularly efficient implementation,
        // but it doesn't need to be done very often.
    }

    // Used to create a temporary connection between a slot and a signal.
    // When this is deleted, the signal will be disconnected.
    Component {
        id: slotComponent
        QtObject {
            required property var slot
            // Just in case something goes wrong and we need to debug this.
            objectName: `Slot_${this.toString()}`
        }
    }

    function setNotificationMessage(text = "",
                                    animate = text === notificationsLabel.text)
    {
        notificationsLabel.notificationText = text
        if (text) {
            notificationResetTimer.start()
        }
        if (animate) {
            bounceAnimation.start()
        }
    }

    function focusFirstLoginFormItem() {
        if (usernameField?.visible) {
            usernameField.forceActiveFocus(Qt.TabFocusReason)
        } else if (passwordField?.visible) {
            passwordField.forceActiveFocus(Qt.TabFocusReason)
        } else if (loginButton?.visible) {
            loginButton.forceActiveFocus(Qt.TabFocusReason)
        } else {
            root.contentItem.nextItemInFocusChain().forceActiveFocus(Qt.TabFocusReason)
        }
    }

    T.StackView.onActivating: {
        Qt.callLater(root.focusFirstLoginFormItem);
    }

    // Don't allow input while waiting for a response
    enabled: !waitingForResponse

    // Where the bottom of the login prompt items would be
    baselineOffset: contentItem.y + contentItem.baselineOffset

    // implicitHeaderWidth/Height and implicitFooterWidth/Height are 0 when
    // header/footer isn't visible. We want to always use the implicit size.
    implicitWidth: Math.max(contentWidth + leftPadding + rightPadding,
                            header?.implicitWidth ?? 0,
                            footer?.implicitWidth ?? 0)
    implicitHeight: contentHeight + topPadding + bottomPadding
                    + (header?.implicitHeight ?? 0 > 0 ? header.implicitHeight + spacing : 0)
                    + (footer?.implicitHeight ?? 0 > 0 ? footer.implicitHeight + spacing : 0)

    header: UserList {
        id: userListView
        // hide if not enough space
        visible: root.showUserList && root.height >= root.implicitHeight
        onUserSelected: focusFirstLoginFormItem()
    }

    spacing: Kirigami.Units.largeSpacing

    KeyboardIndicator.KeyState {
        id: capsLockState
        key: Qt.Key_CapsLock
    }

    PlasmaComponents3.Label {
        id: notificationsLabel
        property string notificationText: ""
        parent: root
        anchors.horizontalCenter: parent.horizontalCenter
        y: root.contentItem.y - root.topPadding
        bottomPadding: root.spacing
        width: Math.min(parent.width, implicitWidth)
        horizontalAlignment: Text.AlignHCenter
        textFormat: Text.PlainText
        wrapMode: Text.WordWrap
        font.italic: true
        text: {
            const parts = [];
            if (capsLockState.locked) {
                parts.push(i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Caps Lock is on"));
            }
            if (notificationText) {
                parts.push(notificationText);
            }
            return parts.join(" • ");
        }

        SequentialAnimation {
            id: bounceAnimation
            loops: 1
            PropertyAnimation {
                target: notificationsLabel
                properties: "scale"
                from: 1.0
                to: 1.1
                duration: Kirigami.Units.longDuration
                easing.type: Easing.OutQuad
            }
            PropertyAnimation {
                target: notificationsLabel
                properties: "scale"
                from: 1.1
                to: 1.0
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InQuad
            }
        }
        Timer {
            id: notificationResetTimer
            interval: 3000
            onTriggered: root.setNotificationMessage()
        }
    }

    topPadding: notificationsLabel.implicitHeight

    //goal is to show the prompts, in ~16 grid units high, then the action buttons
    //but collapse the space between the prompts and actions if there's no room
    //ui is constrained to 16 grid units wide, or the screen
    contentItem: ColumnLayout {
        baselineOffset: loginForm.y + loginForm.baselineOffset
        spacing: root.spacing
        LoginForm {
            id: loginForm
            focus: true
            Layout.alignment: Qt.AlignCenter
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumWidth: Layout.preferredWidth
            Layout.maximumHeight: implicitHeight
            // Try to line up with the centers of user avatars to the left and right.
            Layout.preferredWidth: Math.max(implicitWidth, root.userList.delegateSize.width * 2)

            usernameField.visible: !root.userList.visible

            passwordField.visible: usernameField.visible
                || root.userList.selectedUserNeedsPassword

            passwordField.Keys.onEscapePressed: {
                root.userList.forceActiveFocus(Qt.ShortcutFocusReason);
            }
            //if empty and left or right is pressed change selection in user switch
            //this cannot be in keys.onLeftPressed as then it doesn't reach the password box
            passwordField.Keys.onPressed: event => {
                if (event.key === Qt.Key_Left && passwordField.length === 0) {
                    root.userList.decrementCurrentIndex();
                    event.accepted = true
                }
                if (event.key === Qt.Key_Right && passwordField.length === 0) {
                    root.userList.incrementCurrentIndex();
                    event.accepted = true
                }
            }

            onLoginRequested: root.requestLogin()
        }
    }

    footer: T.ToolBar { // Using ToolBar for accessibility info
        visible: root.height >= root.implicitHeight - root.header.implicitHeight - root.spacing
        implicitWidth: contentWidth
        implicitHeight: Math.max(contentHeight, userListView.implicitHeight - root.topPadding)
        Row { // A child of the contentItem
            id: actionItemsLayout
            anchors.centerIn: parent
            height: Math.min(parent.height, implicitHeight)
            width: Math.min(parent.width, implicitWidth)
            spacing: root.spacing
        }
    }
}
