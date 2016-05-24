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
        mainStack.pop()
        if (searching) {
            mainStack.push(usernameInput);
        } else {
            mainStack.push(userView);
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

            UserDelegate {
                anchors.bottom: userNameInput.top
                anchors.horizontalCenter: parent.horizontalCenter
                iconSource: "user-none"
            }

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
        sddm.login(mainStack.currentItem.userName, passwordBox.text, root.sessionIndex)
    }

    Connections {
        target: sddm
        onLoginFailed: {
            root.loggingIn = false
            root.notificationMessage = i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Login Failed")
            notificationResetTimer.start();
        }
    }

    //Resets the "Login Failed" notification after n seconds
    Timer {
        id: notificationResetTimer
        interval: 5000
        onTriggered: root.notificationMessage = ""
    }


    //main bit:
    StackView {
        id: mainStack
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: passwordBox.top
        anchors.bottomMargin: units.smallSpacing

        initialItem: searching ? usernameInput : userView
    }

    PlasmaComponents.TextField {
        id: passwordBox
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: units.largeSpacing * 5
        placeholderText: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Password")
        focus: true

        enabled: !loggingIn
        echoMode: TextInput.Password

        onAccepted: startLogin()

        Keys.onEscapePressed: {
            mainStack.currentItem.forceActiveFocus();
        }

        //if empty and left or right is pressed change selection in user switch
        //this cannot be in keys.onLeftPressed as then it doesn't reach the password box
        Keys.onPressed: {
            if (text.length > 0) {
                return;
            }

            if (event.key == Qt.Key_Left && mainStack.currentItem.decrementCurrentIndex) {
                mainStack.currentItem.decrementCurrentIndex();
                event.accepted = true
            }
            if (event.key == Qt.Key_Right && mainStack.currentItem.incrementCurrentIndex) {
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

        text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Login")
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
