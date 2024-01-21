/*
    Copyright 2020  Devin Lin <espidev@gmail.com>
    Copyright 2023  Nicolas Fella <nicolas.fella@gmx.de>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Shapes
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami
import FingerprintModel

Kirigami.Dialog {

    enum DialogState {
        FingerprintList,
        PickFinger,
        Enrolling,
        EnrollComplete
    }

    visible: true

    title: i18n("Configure Fingerprints")
    standardButtons: QQC2.Dialog.NoButton

    implicitWidth: Kirigami.Units.gridUnit * 20
    implicitHeight: Kirigami.Units.gridUnit * 18

    ColumnLayout {
        anchors.fill: parent

        Kirigami.InlineMessage {
            type: Kirigami.MessageType.Error
            visible: kcm.fingerprintModel.currentError !== ""
            text: kcm.fingerprintModel.currentError
            Layout.fillWidth: true
            actions: [
                Kirigami.Action {
                    icon.name: "dialog-close"
                    onTriggered: kcm.fingerprintModel.currentError = ""
                }
            ]
        }

        QQC2.StackView {
            id: stack

            Layout.fillWidth: true
            Layout.fillHeight: true

            initialItem: fingerPrints
        }
    }

    Component {
        id: fingerPrints

        FingerprintsList {}
    }

    Component {
        id: pickFinger

        PickFinger {}
    }

    Component {
        id: enrolling

        EnrollFeedback {}
    }

    Connections {
        target: kcm.fingerprintModel

        function onDialogStateChanged() {
            if (kcm.fingerprintModel.dialogState == FingerprintDialog.FingerprintList) {
                stack.clear()
                stack.push(fingerPrints)
            }

            if (kcm.fingerprintModel.dialogState == FingerprintDialog.Enrolling) {
                stack.push(enrolling)
            }
        }
    }

    customFooterActions: [
        // FingerprintList State
        Kirigami.Action {
            text: i18nc("@action:button 'all' refers to fingerprints", "Clear All")
            visible: kcm.fingerprintModel.dialogState === FingerprintDialog.DialogState.FingerprintList
            enabled: kcm.fingerprintModel.enrolledFingerprints.length !== 0
            icon.name: "delete"
            onTriggered: kcm.fingerprintModel.clearFingerprints();
        },
        Kirigami.Action {
            text: i18n("Add")
            visible: kcm.fingerprintModel.dialogState === FingerprintDialog.DialogState.FingerprintList
            enabled: kcm.fingerprintModel.availableFingersToEnroll.length !== 0
            icon.name: "list-add"
            onTriggered: {
                kcm.fingerprintModel.dialogState = FingerprintDialog.DialogState.PickFinger
                stack.push(pickFinger)
            }
        },

        // Enrolling State
        Kirigami.Action {
            text: i18n("Cancel")
            visible: kcm.fingerprintModel.dialogState === FingerprintDialog.DialogState.Enrolling
            icon.name: "dialog-cancel"
            onTriggered: kcm.fingerprintModel.stopEnrolling();
        },

        // EnrollComplete State
        Kirigami.Action {
            text: i18n("Done")
            visible: kcm.fingerprintModel.dialogState === FingerprintDialog.DialogState.EnrollComplete
            icon.name: "dialog-ok"
            onTriggered: kcm.fingerprintModel.stopEnrolling();
        }
    ]
}
