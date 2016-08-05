import QtQuick 2.2

import QtQuick.Layouts 1.1
import QtQuick.Controls 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

Item {
    id: root

    property bool searching: false
    property bool loggingIn: false
    property string notificationMessage
    property int sessionIndex

    onSearchingChanged: {
        if (searching) {
            mainStack.push(usernameInput);
        } else {
            mainStack.pop();
        }
        mainStack.currentItem.forceActiveFocus();
    }

    Component {
        id: userView
        //even though stackview is a scope in itself we need a spacer item to align the user list
        //so that components are always the same height.
        //if the stackview changes height the animation looks weird

        //FocusScope rather than Item to focus proxy onto the user list
        FocusScope {
            function incrementCurrentIndex() {
                userList.incrementCurrentIndex();
            }

            function decrementCurrentIndex() {
                userList.decrementCurrentIndex()
            }

            property alias userName: userList.selectedUser

            UserList {
                id: userList
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                model: userModel
                focus: true
                onUserSelected: nextItemInFocusChain().forceActiveFocus()
            }
        }
    }

    Component {
        id: usernameInput
        FocusScope {
            property alias userName: userNameInput.text

            PlasmaComponents.TextField {
                id: userNameInput
                focus: true
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                placeholderText: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Username");
            }
        }
    }

    function startLogin() {
        root.loggingIn = true
        root.notificationMessage = ""
        sddm.login(mainStack.currentItem.userName, passwordBox.text, sessionButton.currentIndex)
    }

    Connections {
        target: sddm
        onLoginFailed: {
            root.loggingIn = false
            root.notificationMessage = i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Login Failed")
        }
    }


    //main bit:
    StackView {
        id: mainStack
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: passwordBox.top
        anchors.bottomMargin: units.smallSpacing

        initialItem: userView
    }

    PlasmaComponents.TextField {
        id: passwordBox
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: units.largeSpacing * 5
        placeholderText: "Password"
        focus: true
        echoMode: TextInput.Password

        enabled: !loggingIn

        onAccepted: startLogin()

        Keys.onEscapePressed: {
            mainStack.currentItem.forceActiveFocus();
        }

        //if empty and left or right is pressed change selection in user switch
        //this cannot be in keys.onLeftPressed as then it doesn't reach the password box
        Keys.onPressed: {
            if (event.key == Qt.Key_Left && !text && mainStack.currentItem.decrementCurrentIndex) {
                mainStack.currentItem.decrementCurrentIndex();
                event.accepted = true
            }
            if (event.key == Qt.Key_Right && !text && mainStack.currentItem.incrementCurrentIndex) {
                mainStack.currentItem.incrementCurrentIndex();
                event.accepted = true
            }
        }
    }

    PlasmaComponents.Button {
        id: loginButton
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: passwordBox.bottom
        anchors.topMargin: units.smallSpacing

        text: "Login"
        enabled: !loggingIn

        onClicked: startLogin();
    }

    PlasmaComponents.Label {
        id: notificationsLabel
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: loginButton.bottom
        anchors.topMargin: units.smallSpacing
        text: root.notificationMessage
    }
}
