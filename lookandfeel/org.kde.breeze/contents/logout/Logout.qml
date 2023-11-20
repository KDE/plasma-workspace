/*
    SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
    SPDX-FileCopyrightText: 2023 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.2
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.12 as QQC2

import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.coreaddons 1.0 as KCoreAddons
import org.kde.kirigami 2.20 as Kirigami

import org.kde.breeze.components
import "timer.js" as AutoTriggerTimer

import org.kde.plasma.private.sessions 2.0

Item {
    id: root
    Kirigami.Theme.inherit: false
    Kirigami.Theme.colorSet: Kirigami.Theme.Complementary

    property alias backgroundColor: backgroundRect.color

    property real timeout: 30
    property real remainingTime: root.timeout

    property var currentAction

    KCoreAddons.KUser {
        id: kuser
    }

    // For showing a "other users are logged in" hint
    SessionsModel {
        id: sessionsModel
        includeUnusedSessions: false
    }

    SessionManagement {
        id: sessionManagement
    }

    QQC2.Action {
        onTriggered: Qt.quit()
        shortcut: "Escape"
    }

    onRemainingTimeChanged: {
        if (remainingTime <= 0) {
            root.currentAction();
        }
    }

    Timer {
        id: countDownTimer
        running: true
        repeat: true
        interval: 1000
        onTriggered: remainingTime--
        Component.onCompleted: {
            AutoTriggerTimer.addCancelAutoTriggerCallback(function() {
                countDownTimer.running = false;
            });
        }
    }

    Component.onCompleted: {
        switch (defaultAction) {
        case "reboot":
            rebootButton.focus = true;
            root.currentAction = rebootButton.action;
            break;
        case "shutdown":
            shutdownButton.focus = true;
            root.currentAction = shutdownButton.action;
            break;
        default:
            logoutButton.focus = true;
            root.currentAction = logoutButton.action;
            break;
        }
    }

    function isLightColor(color) {
        return Math.max(color.r, color.g, color.b) > 0.5
    }

    Rectangle {
        id: backgroundRect
        anchors.fill: parent
        //use "black" because this is intended to look like a general darkening of the scene. a dark gray as normal background would just look too "washed out"
        color: root.isLightColor(Kirigami.Theme.backgroundColor) ? Kirigami.Theme.backgroundColor : "black"
        opacity: 0.5
    }
    MouseArea {
        anchors.fill: parent
        onClicked: Qt.quit
    }
    UserDelegate {
        width: Kirigami.Units.gridUnit * 8
        height: Kirigami.Units.gridUnit * 9
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.verticalCenter
        }
        constrainText: false
        avatarPath: kuser.faceIconUrl
        iconSource: "user-identity"
        isCurrent: true
        name: kuser.fullName
    }
    ColumnLayout {
        anchors {
            top: parent.verticalCenter
            topMargin: Kirigami.Units.gridUnit * 2
            horizontalCenter: parent.horizontalCenter
        }
        spacing: Kirigami.Units.largeSpacing

        height: Math.max(implicitHeight, Kirigami.Units.gridUnit * 10)
        width: Math.max(implicitWidth, Kirigami.Units.gridUnit * 16)

        PlasmaComponents.Label {
            font.pointSize: Kirigami.Theme.defaultFont.pointSize + 1
            Layout.maximumWidth: Math.max(Kirigami.Units.gridUnit * 16, logoutButtonsRow.implicitWidth)
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.italic: true
            text: i18ndp("plasma_lookandfeel_org.kde.lookandfeel",
                         "One other user is currently logged in. If the computer is shut down or restarted, that user may lose work.",
                         "%1 other users are currently logged in. If the computer is shut down or restarted, those users may lose work.",
                         sessionsModel.count - 1)
            visible: sessionsModel.count > 1
        }

        PlasmaComponents.Label {
            font.pointSize: Kirigami.Theme.defaultFont.pointSize + 1
            Layout.maximumWidth: Math.max(Kirigami.Units.gridUnit * 16, logoutButtonsRow.implicitWidth)
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.italic: true
            text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "When restarted, the computer will enter the firmware setup screen.")
            visible: rebootToFirmwareSetup
        }

        RowLayout {
            id: logoutButtonsRow
            spacing: Kirigami.Units.gridUnit * 2
            Layout.alignment: Qt.AlignHCenter
            LogoutButton {
                id: suspendButton
                iconSource: "system-suspend"
                text: i18ndc("plasma_lookandfeel_org.kde.lookandfeel", "Suspend to RAM", "Sleep")
                action: function() {
                    sessionManagement.suspend();
                    Qt.quit();
                }                KeyNavigation.left: logoutButton
                KeyNavigation.right: hibernateButton
                KeyNavigation.down: okButton
                visible: sessionManagement.canSuspend
            }
            LogoutButton {
                id: hibernateButton
                iconSource: "system-suspend-hibernate"
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Hibernate")
                action: function() {
                    sessionManagement.hibernate();
                    Qt.quit();
                }
                KeyNavigation.left: suspendButton
                KeyNavigation.right: rebootButton
                KeyNavigation.down: okButton
                visible: sessionManagement.canHibernate
            }
            LogoutButton {
                id: rebootButton
                iconSource: "system-reboot"
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Restart")
                action: function() {
                    sessionManagement.requestReboot(SessionManagement.Skip);
                    Qt.quit();
                }
                KeyNavigation.left: hibernateButton
                KeyNavigation.right: shutdownButton
                KeyNavigation.down: okButton
                visible: sessionManagement.canReboot
            }
            LogoutButton {
                id: shutdownButton
                iconSource: "system-shutdown"
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Shut Down")
                action: function() {
                    sessionManagement.requestShutdown(SessionManagement.Skip);
                    Qt.quit();
                }
                KeyNavigation.left: rebootButton
                KeyNavigation.right: logoutButton
                KeyNavigation.down: okButton
                visible: sessionManagement.canShutdown
            }
            LogoutButton {
                id: logoutButton
                iconSource: "system-log-out"
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Log Out")
                action: function() {
                    sessionManagement.requestLogout(SessionManagement.Skip);
                    Qt.quit();
                }
                KeyNavigation.left: shutdownButton
                KeyNavigation.right: suspendButton
                KeyNavigation.down: okButton
                visible: sessionManagement.canLogout
            }
        }

        PlasmaComponents.Label {
            font.pointSize: Kirigami.Theme.defaultFont.pointSize + 1
            Layout.alignment: Qt.AlignHCenter
            //opacity, as visible would re-layout
            opacity: countDownTimer.running ? 1 : 0
            Behavior on opacity {
                OpacityAnimator {
                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.InOutQuad
                }
            }
            text: {
                switch (defaultAction) {
                    case "reboot":
                        return i18ndp("plasma_lookandfeel_org.kde.lookandfeel", "Restarting in 1 second", "Restarting in %1 seconds", root.remainingTime);
                    case "shutdown":
                        return i18ndp("plasma_lookandfeel_org.kde.lookandfeel", "Shutting down in 1 second", "Shutting down in %1 seconds", root.remainingTime);
                    default:
                        return i18ndp("plasma_lookandfeel_org.kde.lookandfeel", "Logging out in 1 second", "Logging out in %1 seconds", root.remainingTime);
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            PlasmaComponents.Button {
                id: okButton
                implicitWidth: Kirigami.Units.gridUnit * 6
                font.pointSize: Kirigami.Theme.defaultFont.pointSize + 1
                enabled: root.currentAction !== null
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "OK")
                onClicked: root.currentAction.action()
                Keys.onEnterPressed: root.currentAction.action()
                Keys.onReturnPressed: root.currentAction.action()
                KeyNavigation.left: cancelButton
                KeyNavigation.right: cancelButton
                KeyNavigation.up: suspendButton
            }
            PlasmaComponents.Button {
                id: cancelButton
                implicitWidth: Kirigami.Units.gridUnit * 6
                font.pointSize: Kirigami.Theme.defaultFont.pointSize + 1
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Cancel")
                onClicked: Qt.quit()
                Keys.onEnterPressed: Qt.quit()
                Keys.onReturnPressed: Qt.quit()
                KeyNavigation.left: okButton
                KeyNavigation.right: okButton
                KeyNavigation.up: suspendButton
            }
        }
    }
}
