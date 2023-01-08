/*
    Copyright 2020  Devin Lin <espidev@gmail.com>

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

import QtQuick 2.12
import QtQuick.Dialogs 1.1
import QtQuick.Layouts 1.3
import QtQuick.Shapes 1.12
import QtQuick.Controls 2.5 as QQC2

import org.kde.kirigami 2.12 as Kirigami
import org.kde.plasma.kcm.users 1.0
import FingerprintModel 1.0

Kirigami.OverlaySheet {
    id: fingerprintRoot

    property var account
    property var fingerprintModel: kcm.fingerprintModel
    property string currentFinger

    enum DialogState {
        FingerprintList,
        PickFinger,
        Enrolling,
        EnrollComplete
    }

    function openAndClear() {
        fingerprintModel.switchUser(account.name == kcm.userModel.getLoggedInUser().name ? "" : account.name);
        this.open();
    }

    onSheetOpenChanged: {
        if (sheetOpen && fingerprintModel.currentlyEnrolling) {
            fingerprintModel.stopEnrolling();
        }
    }

    header: Kirigami.Heading {
        level: 2
        text: i18n("Configure Fingerprints")
    }

    footer: Kirigami.ActionToolBar {
        flat: false
        alignment: Qt.AlignRight

        actions: [
            // FingerprintList State
            Kirigami.Action {
                text: i18nc("@action:button 'all' refers to fingerprints", "Clear All")
                visible: fingerprintModel.dialogState === FingerprintDialog.DialogState.FingerprintList
                enabled: fingerprintModel.enrolledFingerprints.length !== 0
                icon.name: "delete"
                onTriggered: fingerprintModel.clearFingerprints();
            },
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

    contentItem: Item {
        id: rootPanel
        implicitWidth: Kirigami.Units.gridUnit * 12
        Layout.maximumWidth: Kirigami.Units.gridUnit * 24
        Layout.leftMargin: Kirigami.Units.smallSpacing
        Layout.rightMargin: Kirigami.Units.smallSpacing
        height: Kirigami.Units.gridUnit * 18

        ColumnLayout {
            id: enrollFeedback
            spacing: Kirigami.Units.largeSpacing * 2
            visible: fingerprintModel.dialogState === FingerprintDialog.DialogState.Enrolling || fingerprintModel.dialogState === FingerprintDialog.DialogState.EnrollComplete
            anchors.fill: parent

            Kirigami.Heading {
                level: 2
                text: i18n("Enrolling Fingerprint")
                Layout.alignment: Qt.AlignHCenter
                visible: fingerprintModel.dialogState === FingerprintDialog.DialogState.Enrolling
            }

            QQC2.Label {
                text: {
                    if (fingerprintModel.scanType == FprintDevice.Press) {
                        if (fingerprintRoot.currentFinger == "right-index-finger") {
                            return i18n("Please repeatedly press your right index finger on the fingerprint sensor.")
                        } else if (fingerprintRoot.currentFinger == "right-middle-finger") {
                            return i18n("Please repeatedly press your right middle finger on the fingerprint sensor.")
                        } else if (fingerprintRoot.currentFinger == "right-ring-finger") {
                            return i18n("Please repeatedly press your right ring finger on the fingerprint sensor.")
                        } else if (fingerprintRoot.currentFinger == "right-little-finger") {
                            return i18n("Please repeatedly press your right little finger on the fingerprint sensor.")
                        } else if (fingerprintRoot.currentFinger == "right-thumb") {
                            return i18n("Please repeatedly press your right thumb on the fingerprint sensor.")
                        } else if (fingerprintRoot.currentFinger == "left-index-finger") {
                            return i18n("Please repeatedly press your left index finger on the fingerprint sensor.")
                        } else if (fingerprintRoot.currentFinger == "left-middle-finger") {
                            return i18n("Please repeatedly press your left middle finger on the fingerprint sensor.")
                        } else if (fingerprintRoot.currentFinger == "left-ring-finger") {
                            return i18n("Please repeatedly press your left ring finger on the fingerprint sensor.")
                        } else if (fingerprintRoot.currentFinger == "left-little-finger") {
                            return i18n("Please repeatedly press your left little finger on the fingerprint sensor.")
                        } else if (fingerprintRoot.currentFinger == "left-thumb") {
                            return i18n("Please repeatedly press your left thumb on the fingerprint sensor.")
                        }
                    } else if (fingerprintModel.scanType == FprintDevice.Swipe) {
                        if (fingerprintRoot.currentFinger == "right-index-finger") {
                            return i18n("Please repeatedly swipe your right index finger on the fingerprint sensor.")
                        } else if (fingerprintRoot.currentFinger == "right-middle-finger") {
                            return i18n("Please repeatedly swipe your right middle finger on the fingerprint sensor.")
                        } else if (fingerprintRoot.currentFinger == "right-ring-finger") {
                            return i18n("Please repeatedly swipe your right ring finger on the fingerprint sensor.")
                        } else if (fingerprintRoot.currentFinger == "right-little-finger") {
                            return i18n("Please repeatedly swipe your right little finger on the fingerprint sensor.")
                        } else if (fingerprintRoot.currentFinger == "right-thumb") {
                            return i18n("Please repeatedly swipe your right thumb on the fingerprint sensor.")
                        } else if (fingerprintRoot.currentFinger == "left-index-finger") {
                            return i18n("Please repeatedly swipe your left index finger on the fingerprint sensor.")
                        } else if (fingerprintRoot.currentFinger == "left-middle-finger") {
                            return i18n("Please repeatedly swipe your left middle finger on the fingerprint sensor.")
                        } else if (fingerprintRoot.currentFinger == "left-ring-finger") {
                            return i18n("Please repeatedly swipe your left ring finger on the fingerprint sensor.")
                        } else if (fingerprintRoot.currentFinger == "left-little-finger") {
                            return i18n("Please repeatedly swipe your left little finger on the fingerprint sensor.")
                        } else if (fingerprintRoot.currentFinger == "left-thumb") {
                            return i18n("Please repeatedly swipe your left thumb on the fingerprint sensor.")
                        }
                    }
                }

                Layout.alignment: Qt.AlignHCenter
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                Layout.maximumWidth: parent.width
                visible: fingerprintModel.dialogState === FingerprintDialog.DialogState.Enrolling
            }

            Kirigami.Heading {
                level: 2
                text: i18n("Finger Enrolled")
                Layout.alignment: Qt.AlignHCenter
                visible: fingerprintModel.dialogState === FingerprintDialog.DialogState.EnrollComplete
            }

            // reset from back from whatever color was used before
            onVisibleChanged: progressCircle.colorTimer.restart();

            // progress circle
            FingerprintProgressCircle {
                id: progressCircle
            }

            QQC2.Label {
                text: fingerprintModel.enrollFeedback
                wrapMode: Text.Wrap
                Layout.maximumWidth: parent.width
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            }
        }

        ColumnLayout {
            id: pickFinger
            visible: fingerprintModel.dialogState === FingerprintDialog.DialogState.PickFinger
            anchors.centerIn: parent
            spacing: Kirigami.Units.largeSpacing
            width: parent.width

            Kirigami.Heading {
                level: 2
                text: i18n("Pick a finger to enroll")
                Layout.alignment: Qt.AlignHCenter
            }

            Item {
                id: handContainer
                implicitHeight: basePalm.height
                Layout.fillWidth: true

                property string currentFinger: ""
                property string currentFingerData: ""

                Image {
                    id: basePalm
                    source: kcm.recolorSVG(Qt.resolvedUrl("hand-images/palm.svg"), Kirigami.Theme.textColor)
                    fillMode: Image.PreserveAspectFit
                    width: handContainer.width
                    opacity: 0.25
                }

                Repeater {
                    model: fingerprintModel.availableFingersToEnroll
                    delegate: Image {
                        id: img
                        activeFocusOnTab: true
                        source: kcm.recolorSVG(Qt.resolvedUrl(`hand-images/${modelData.internalName}.svg`), color)
                        readonly property color color: focus ?
                            Kirigami.Theme.focusColor :
                            (maskArea.hovered ? Kirigami.Theme.hoverColor : Kirigami.Theme.textColor)

                        fillMode: Image.PreserveAspectFit
                        anchors.fill: parent
                        Accessible.name: modelData.friendlyName
                        Accessible.focusable: true
                        Accessible.role: Accessible.RadioButton
                        Accessible.onPressAction: {
                            img.activate()
                        }
                        Keys.onEnterPressed: {
                            img.activate()
                        }
                        function activate() {
                            fingerprintRoot.currentFinger = modelData.internalName;
                            fingerprintModel.startEnrolling(modelData.internalName);
                        }
                        MaskMouseArea {
                            id: maskArea
                            anchors.fill: parent
                            onTapped: {
                                img.activate()
                            }
                        }
                    }
                }

                Repeater {
                    model: fingerprintModel.unavailableFingersToEnroll
                    delegate: Image {
                        source: kcm.recolorSVG(Qt.resolvedUrl(`hand-images/${modelData.internalName}.svg`), Kirigami.Theme.textColor)
                        fillMode: Image.PreserveAspectFit
                        anchors.fill: parent
                        opacity: 0.25
                    }
                }
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
                        iconName: "dialog-close"
                        onTriggered: fingerprintModel.currentError = ""
                    }
                ]
            }

            ListView {
                id: fingerprintsList
                model: kcm.fingerprintModel.deviceFound ? fingerprintModel.enrolledFingerprints : 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                QQC2.ScrollBar.vertical: QQC2.ScrollBar {}

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
                            text: finger.friendlyName
                        }
                    }
                    actions: [
                        Kirigami.Action {
                            iconName: "edit-entry"
                            onTriggered: {
                                fingerprintRoot.currentFinger = finger.internalName;
                                fingerprintModel.startEnrolling(finger.internalName);
                            }
                            tooltip: i18n("Re-enroll finger")
                        },
                        Kirigami.Action {
                            iconName: "entry-delete"
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
}

