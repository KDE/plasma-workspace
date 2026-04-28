/*
    SPDX-FileCopyrightText: 2020 Devin Lin <espidev@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Shapes
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami

QQC2.Dialog {
    id: root

    property var fingerprintModel: kcm.fingerprintModel
    property string currentFinger

    enum DialogState {
        FingerprintList,
        PickFinger,
        Enrolling,
        EnrollComplete
    }

    title: i18n("Configure Fingerprints")

    visible: true

    onRejected: root.fingerprintModel.stopEnrolling()

    standardButtons: doneAction.visible ? QQC2.Dialog.NoButton : QQC2.Dialog.Cancel

    property list<Kirigami.Action> customFooterActions: [
        // FingerprintList State
        Kirigami.Action {
            text: i18n("Add")
            visible: root.fingerprintModel.dialogState === FingerprintDialog.DialogState.FingerprintList
            enabled: root.fingerprintModel.availableFingersToEnroll.length !== 0
            icon.name: "list-add"
            onTriggered: {
                root.fingerprintModel.dialogState = FingerprintDialog.DialogState.PickFinger;
                stack.push(pickFinger);
            }
        },

        // EnrollComplete State
        Kirigami.Action {
            id: doneAction
            text: i18n("Done")
            visible: root.fingerprintModel.dialogState === FingerprintDialog.DialogState.EnrollComplete
            icon.name: "dialog-ok"
            onTriggered: root.fingerprintModel.stopEnrolling()
        }
    ]

    footer: QQC2.DialogButtonBox {
        Layout.fillWidth: true

        Repeater {
            id: customFooterButtons

            // DialogButtonBox should NOT contain invisible buttons, because in Qt 6
            // ListView preserves space even for invisible items.
            model: root.customFooterActions.filter(action => action.visible)
            // we have to use Button instead of ToolButton, because ToolButton has no visual distinction when disabled
            delegate: QQC2.Button {
                required property Kirigami.Action modelData

                flat: root.flatFooterButtons
                action: modelData
            }
        }
    }

    ColumnLayout {

        Kirigami.InlineMessage {
            type: Kirigami.MessageType.Error
            visible: root.fingerprintModel.currentError !== ""
            text: root.fingerprintModel.currentError
            Layout.fillWidth: true
            actions: [
                Kirigami.Action {
                    icon.name: "dialog-close"
                    onTriggered: root.fingerprintModel.currentError = ""
                }
            ]
        }

        QQC2.StackView {
            id: stack

            Layout.minimumWidth: Kirigami.Units.gridUnit * 20
            Layout.minimumHeight: Kirigami.Units.gridUnit * 18
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            initialItem: fingerPrints
        }
    }

    Connections {
        target: root.fingerprintModel

        function onDialogStateChanged() {
            if (root.fingerprintModel.dialogState == FingerprintDialog.FingerprintList) {
                stack.clear();
                stack.push(fingerPrints);
            }
            if (root.fingerprintModel.dialogState == FingerprintDialog.Enrolling) {
                stack.push(enrolling);
            }

            if (root.fingerprintModel.dialogState == FingerprintDialog.EnrollComplete) {
                stack.push(enrolledConfirmation);
            }
        }
    }

    Component {
        id: enrolling

        EnrollFeedback {
            enrollFeedback: root.fingerprintModel.enrollFeedback
            scanType: root.fingerprintModel.scanType
            finger: root.currentFinger
        }
    }

    Component {
        id: pickFinger

        PickFinger {
            availableFingers: root.fingerprintModel.availableFingersToEnroll
            unavailableFingers: root.fingerprintModel.unavailableFingersToEnroll

            onFingerPicked: finger => {
                root.currentFinger = finger;
                root.fingerprintModel.startEnrolling(finger);
            }
        }
    }

    Component {
        id: fingerPrints

        FingerprintList {
            model: root.fingerprintModel.enrolledFingerprints

            onReenrollFinger: finger => {
                root.currentFinger = finger;
                root.fingerprintModel.startEnrolling(finger);
            }

            onDeleteFinger: finger => {
                root.fingerprintModel.deleteFingerprint(finger);
            }
        }
    }

    Component {
        id: enrolledConfirmation

        EnrolledConfirmation {}
    }
}
