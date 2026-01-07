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

Kirigami.Dialog {
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

    standardButtons: QQC2.Dialog.NoButton

    customFooterActions: [
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

        // Enrolling State
        Kirigami.Action {
            text: i18n("Cancel")
            visible: root.fingerprintModel.dialogState === FingerprintDialog.DialogState.Enrolling
            icon.name: "dialog-cancel"
            onTriggered: root.fingerprintModel.stopEnrolling()
        },

        // EnrollComplete State
        Kirigami.Action {
            text: i18n("Done")
            visible: root.fingerprintModel.dialogState === FingerprintDialog.DialogState.EnrollComplete
            icon.name: "dialog-ok"
            onTriggered: root.fingerprintModel.stopEnrolling()
        }
    ]

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
