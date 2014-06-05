import QtQuick 2.1
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

Item {
    height: usersList.height
    
    property alias model: usersList.model
    property alias selectedUser: usersList.selectedUser
    
    InfoPane {
        id: infoPane
        anchors {
            verticalCenter: usersList.verticalCenter
            right: usersList.left
            left: parent.left
        }
    }

    UserList {
        id: usersList

        Rectangle {//debug
            visible: debug
            border.color: "red"
            border.width: 1
            anchors.fill: parent
            color: "#00000000"
            z:-1000
        }

        anchors {
            top: parent.top
            left: parent.horizontalCenter
            right: parent.right

            leftMargin: -userItemWidth*1.5 //allow 1 item to the left of the centre (the half is to fit the item that will go in the centre)
        }
        clip: true
        height: userItemHeight
        //           / currentIndex: indexForUserName(greeter.lastLoggedInUser)
        cacheBuffer: 1000

        //highlight the item in the middle. The actual list view starts -1.5 userItemWidths so this moves the highlighted item to the centre
        preferredHighlightBegin: userItemWidth * 1
        preferredHighlightEnd: userItemWidth * 2

        //if the user presses down or enter, focus password
        //if user presses any normal key
        //copy that character pressed to the pasword box and force focus

        //can't use forwardTo as I want to switch focus. Also it doesn't work.
        Keys.onPressed: {
            if (event.key == Qt.Key_Down ||
                    event.key == Qt.Key_Enter ||
                    event.key == Qt.Key_Return) {
                passwordInput.forceActiveFocus();
            } else if (event.key & Qt.Key_Escape) {
                //if special key, do nothing. Qt.Escape is 0x10000000 which happens to be a mask used for all special keys in Qt.
            } else {
                passwordInput.text += event.text;
                passwordInput.forceActiveFocus();
            }
        }

        Component.onCompleted: {
            currentIndex = 0;
        }
    }
}
