/*
    SPDX-FileCopyrightText: 2020 Devin Lin <espidev@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Shapes 1.12
import QtQuick.Controls 2.5 as QQC2

import org.kde.kirigami 2.12 as Kirigami
import org.kde.plasma.kcm.users 1.0 as UsersKCM
import FingerprintModel 1.0

Kirigami.OverlaySheet {
    id: fingerprintRoot

    property var fingerprintModel: kcm.fingerprintModel
    property string currentFinger

    enum DialogState {
        FingerprintList,
        PickFinger,
        Enrolling,
        EnrollComplete
    }

    title: i18n("Configure Fingerprints")

    footer: Kirigami.ActionToolBar {
        flat: false
        alignment: Qt.AlignRight

        actions: [
            // FingerprintList State
            Kirigami.Action {
                text: i18n("Add")
                visible: fingerprintModel.dialogState === FingerprintDialog.DialogState.FingerprintList
                enabled: fingerprintModel.availableFingersToEnroll.length !== 0
                icon.name: "list-add"
                onTriggered: fingerprintModel.dialogState = FingerprintDialog.DialogState.PickFinger
            },

            // Enrolling State
            Kirigami.Action {
                text: i18n("Cancel")
                visible: fingerprintModel.dialogState === FingerprintDialog.DialogState.Enrolling
                icon.name: "dialog-cancel"
                onTriggered: fingerprintModel.stopEnrolling();
            },

            // EnrollComplete State
            Kirigami.Action {
                text: i18n("Done")
                visible: fingerprintModel.dialogState === FingerprintDialog.DialogState.EnrollComplete
                icon.name: "dialog-ok"
                onTriggered: fingerprintModel.stopEnrolling();
            }
        ]
    }

    Item {
        id: rootPanel

        implicitWidth: Kirigami.Units.gridUnit * 20
        implicitHeight: Kirigami.Units.gridUnit * 16

        EnrollFeedback {
            anchors.fill: parent

            visible: fingerprintRoot.fingerprintModel.dialogState === FingerprintDialog.DialogState.Enrolling || fingerprintModel.dialogState === FingerprintDialog.DialogState.EnrollComplete

            done: fingerprintRoot.fingerprintModel.dialogState === FingerprintDialog.DialogState.EnrollComplete
            enrollFeedback: fingerprintRoot.fingerprintModel.enrollFeedback
            scanType: fingerprintRoot.fingerprintModel.scanType
            finger: fingerprintRoot.currentFinger
        }

        PickFinger {
            anchors.fill: parent

            visible: fingerprintRoot.fingerprintModel.dialogState === FingerprintDialog.DialogState.PickFinger

            availableFingers: fingerprintRoot.fingerprintModel.availableFingersToEnroll
            unavailableFingers: fingerprintRoot.fingerprintModel.unavailableFingersToEnroll

            onFingerPicked: finger => {
                fingerprintRoot.currentFinger = finger
                fingerprintRoot.fingerprintModel.startEnrolling(finger);
            }
        }

        ColumnLayout {
            id: fingerprints
            spacing: Kirigami.Units.smallSpacing
            visible: fingerprintModel.dialogState === FingerprintDialog.DialogState.FingerprintList
            anchors.fill: parent

            Kirigami.InlineMessage {
                id: errorMessage
                type: Kirigami.MessageType.Error
                visible: fingerprintModel.currentError !== ""
                text: fingerprintModel.currentError
                Layout.fillWidth: true
                actions: [
                    Kirigami.Action {
                        icon.name: "dialog-close"
                        onTriggered: fingerprintModel.currentError = ""
                    }
                ]
            }

            ListView {
                id: fingerprintsList
                model: kcm.fingerprintModel.deviceFound ? fingerprintModel.enrolledFingerprints : 0
                Layout.fillWidth: true
                Layout.fillHeight: true

                delegate: Kirigami.SwipeListItem {
                    property Finger finger: modelData
                    // Don't need a background or hover effect for this use case
                    hoverEnabled: false
                    backgroundColor: "transparent"
                    contentItem: RowLayout {
                        Kirigami.Icon {
                            source: "fingerprint"
                            height: Kirigami.Units.iconSizes.medium
                            width: Kirigami.Units.iconSizes.medium
                        }
                        QQC2.Label {
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            text: finger.friendlyName
                            textFormat: Text.PlainText
                        }
                    }
                    actions: [
                        Kirigami.Action {
                            icon.name: "edit-entry"
                            onTriggered: {
                                fingerprintRoot.currentFinger = finger.internalName;
                                fingerprintModel.startEnrolling(finger.internalName);
                            }
                            tooltip: i18n("Re-enroll finger")
                        },
                        Kirigami.Action {
                            icon.name: "entry-delete"
                            onTriggered: {
                                fingerprintModel.deleteFingerprint(finger.internalName);
                            }
                            tooltip: i18n("Delete fingerprint")
                        }
                    ]
                }

                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - (Kirigami.Units.largeSpacing * 4)
                    visible: fingerprintsList.count == 0
                    text: i18n("No fingerprints added")
                    icon.name: "fingerprint"
                }
            }
        }
    }

    Component.onCompleted: {
        fingerprintButton.dialog = this;
        open();
    }
}
