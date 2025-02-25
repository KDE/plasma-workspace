import QtQuick 2.0
import org.kde.plasma.components 3.0 as PlasmaComponents
import QtQuick.Layouts
import org.greetd

Rectangle {
    anchors.fill: parent
    color: "grey"

    ColumnLayout {
        anchors.centerIn: parent
        PlasmaComponents.TextField {
            id: usernameField
        }
        PlasmaComponents.TextField {
            id: passwordField
            echoMode: TextInput.Password
        }
        PlasmaComponents.Label {
            id: result
        }
        PlasmaComponents.Button {
            text: "login"
            onClicked: function () {
                result.text = "LOGIN STARTED"
                if (Authenticator.authenticate(usernameField.text, passwordField.text)) {
                    result.text = "LOGIN SUCCESSFUL"
                    // we would then do Autenticator.startSession()
                    // probably with an abstraction so we pass the desktop file
                } else {
                    result.text = "LOGIN FAILED"
                }
            }
        }
    }
}
