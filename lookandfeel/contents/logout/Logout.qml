/*
    SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.2
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.12 as QQC2

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.kcoreaddons 1.0 as KCoreAddons

import "../components"
import "timer.js" as AutoTriggerTimer

import org.kde.plasma.private.sessions 2.0

PlasmaCore.ColorScope {
    id: root
    colorGroup: PlasmaCore.Theme.ComplementaryColorGroup
    height: screenGeometry.height
    width: screenGeometry.width

    signal logoutRequested()
    signal haltRequested()
    signal suspendRequested(int spdMethod)
    signal rebootRequested()
    signal rebootRequested2(int opt)
    signal cancelRequested()
    signal lockScreenRequested()

    property alias backgroundColor: backgroundRect.color

    function sleepRequested() {
        root.suspendRequested(2);
    }

    function hibernateRequested() {
        root.suspendRequested(4);
    }
 
    property real timeout: 30
    property real remainingTime: root.timeout
    property var currentAction: {
        switch (sdtype) {
            case ShutdownType.ShutdownTypeReboot:
                return root.rebootRequested;
            case ShutdownType.ShutdownTypeHalt:
                return root.haltRequested;
            default:
                return root.logoutRequested;
        }
    }

    KCoreAddons.KUser {
        id: kuser
    }

    // For showing a "other users are logged in" hint
    SessionsModel {
        id: sessionsModel
        includeUnusedSessions: false
    }

    QQC2.Action {
        onTriggered: root.cancelRequested()
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

    function isLightColor(color) {
        return Math.max(color.r, color.g, color.b) > 0.5
    }

    Rectangle {
        id: backgroundRect
        anchors.fill: parent
        //use "black" because this is intended to look like a general darkening of the scene. a dark gray as normal background would just look too "washed out"
        color: root.isLightColor(PlasmaCore.ColorScope.backgroundColor) ? PlasmaCore.ColorScope.backgroundColor : "black"
        opacity: 0.5
    }
    MouseArea {
        anchors.fill: parent
        onClicked: root.cancelRequested()
    }
    UserDelegate {
        width: PlasmaCore.Units.gridUnit * 7
        height: width
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
            topMargin: PlasmaCore.Units.gridUnit * 2
            horizontalCenter: parent.horizontalCenter
        }
        spacing: PlasmaCore.Units.largeSpacing

        height: Math.max(implicitHeight, PlasmaCore.Units.gridUnit * 10)
        width: Math.max(implicitWidth, PlasmaCore.Units.gridUnit * 16)

        PlasmaComponents.Label {
            font.pointSize: PlasmaCore.Theme.defaultFont.pointSize + 1
            Layout.maximumWidth: PlasmaCore.Units.gridUnit * 16
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.italic: true
            text: i18ndp("plasma_lookandfeel_org.kde.lookandfeel",
                         "One other user is currently logged in. If the computer is shut down or restarted, that user may lose work.",
                         "%1 other users are currently logged in. If the computer is shut down or restarted, those users may lose work.",
                         sessionsModel.count)
            visible: sessionsModel.count > 1
        }

        PlasmaComponents.Label {
            font.pointSize: PlasmaCore.Theme.defaultFont.pointSize + 1
            Layout.maximumWidth: PlasmaCore.Units.gridUnit * 16
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.italic: true
            text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "When restarted, the computer will enter the firmware setup screen.")
            visible: rebootToFirmwareSetup
        }

        RowLayout {
            spacing: PlasmaCore.Units.largeSpacing * 2
            Layout.alignment: Qt.AlignHCenter
            LogoutButton {
                id: suspendButton
                iconSource: "system-suspend"
                text: i18ndc("plasma_lookandfeel_org.kde.lookandfeel", "Suspend to RAM", "Sleep")
                action: root.sleepRequested
                KeyNavigation.left: logoutButton
                KeyNavigation.right: hibernateButton
                visible: spdMethods.SuspendState
            }
            LogoutButton {
                id: hibernateButton
                iconSource: "system-suspend-hibernate"
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Hibernate")
                action: root.hibernateRequested
                KeyNavigation.left: suspendButton
                KeyNavigation.right: rebootButton
                visible: spdMethods.HibernateState
            }
            LogoutButton {
                id: rebootButton
                iconSource: "system-reboot"
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Restart")
                action: root.rebootRequested
                KeyNavigation.left: hibernateButton
                KeyNavigation.right: shutdownButton
                focus: sdtype === ShutdownType.ShutdownTypeReboot
                visible: maysd
            }
            LogoutButton {
                id: shutdownButton
                iconSource: "system-shutdown"
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Shut Down")
                action: root.haltRequested
                KeyNavigation.left: rebootButton
                KeyNavigation.right: logoutButton
                focus: sdtype === ShutdownType.ShutdownTypeHalt
                visible: maysd
            }
            LogoutButton {
                id: logoutButton
                iconSource: "system-log-out"
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Log Out")
                action: root.logoutRequested
                KeyNavigation.left: shutdownButton
                KeyNavigation.right: suspendButton
                focus: sdtype === ShutdownType.ShutdownTypeNone
                visible: canLogout
            }
        }

        PlasmaComponents.Label {
            font.pointSize: PlasmaCore.Theme.defaultFont.pointSize + 1
            Layout.alignment: Qt.AlignHCenter
            //opacity, as visible would re-layout
            opacity: countDownTimer.running ? 1 : 0
            Behavior on opacity {
                OpacityAnimator {
                    duration: PlasmaCore.Units.longDuration
                    easing.type: Easing.InOutQuad
                }
            }
            text: {
                switch (sdtype) {
                    case ShutdownType.ShutdownTypeReboot:
                        return i18ndp("plasma_lookandfeel_org.kde.lookandfeel", "Restarting in 1 second", "Restarting in %1 seconds", root.remainingTime);
                    case ShutdownType.ShutdownTypeHalt:
                        return i18ndp("plasma_lookandfeel_org.kde.lookandfeel", "Shutting down in 1 second", "Shutting down in %1 seconds", root.remainingTime);
                    default:
                        return i18ndp("plasma_lookandfeel_org.kde.lookandfeel", "Logging out in 1 second", "Logging out in %1 seconds", root.remainingTime);
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            PlasmaComponents.Button {
                implicitWidth: PlasmaCore.Units.gridUnit * 6
                font.pointSize: PlasmaCore.Theme.defaultFont.pointSize + 1
                enabled: root.currentAction !== null
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "OK")
                onClicked: root.currentAction()
            }
            PlasmaComponents.Button {
                implicitWidth: PlasmaCore.Units.gridUnit * 6
                font.pointSize: PlasmaCore.Theme.defaultFont.pointSize + 1
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Cancel")
                onClicked: root.cancelRequested()
            }
        }
    }
}
