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

ColumnLayout {
    id: enrollFeedback

    spacing: Kirigami.Units.largeSpacing * 2

    Kirigami.Heading {
        level: 2
        text: kcm.fingerprintModel.dialogState === FingerprintDialog.DialogState.EnrollComplete ? i18n("Finger Enrolled") : i18n("Enrolling Fingerprint")
        Layout.alignment: Qt.AlignHCenter
        // visible: fingerprintModel.dialogState === FingerprintDialog.DialogState.Enrolling
    }

    QQC2.Label {
        text: {
            if (kcm.fingerprintModel.scanType == FprintDevice.Press) {
                if (kcm.fingerprintModel.currentFinger == "right-index-finger") {
                    return i18n("Please repeatedly press your right index finger on the fingerprint sensor.")
                } else if (kcm.fingerprintModel.currentFinger == "right-middle-finger") {
                    return i18n("Please repeatedly press your right middle finger on the fingerprint sensor.")
                } else if (kcm.fingerprintModel.currentFinger == "right-ring-finger") {
                    return i18n("Please repeatedly press your right ring finger on the fingerprint sensor.")
                } else if (kcm.fingerprintModel.currentFinger == "right-little-finger") {
                    return i18n("Please repeatedly press your right little finger on the fingerprint sensor.")
                } else if (kcm.fingerprintModel.currentFinger == "right-thumb") {
                    return i18n("Please repeatedly press your right thumb on the fingerprint sensor.")
                } else if (kcm.fingerprintModel.currentFinger == "left-index-finger") {
                    return i18n("Please repeatedly press your left index finger on the fingerprint sensor.")
                } else if (kcm.fingerprintModel.currentFinger == "left-middle-finger") {
                    return i18n("Please repeatedly press your left middle finger on the fingerprint sensor.")
                } else if (kcm.fingerprintModel.currentFinger == "left-ring-finger") {
                    return i18n("Please repeatedly press your left ring finger on the fingerprint sensor.")
                } else if (kcm.fingerprintModel.currentFinger == "left-little-finger") {
                    return i18n("Please repeatedly press your left little finger on the fingerprint sensor.")
                } else if (kcm.fingerprintModel.currentFinger == "left-thumb") {
                    return i18n("Please repeatedly press your left thumb on the fingerprint sensor.")
                }
            } else if (kcm.fingerprintModel.scanType == FprintDevice.Swipe) {
                if (kcm.fingerprintModel.currentFinger == "right-index-finger") {
                    return i18n("Please repeatedly swipe your right index finger on the fingerprint sensor.")
                } else if (kcm.fingerprintModel.currentFinger == "right-middle-finger") {
                    return i18n("Please repeatedly swipe your right middle finger on the fingerprint sensor.")
                } else if (kcm.fingerprintModel.currentFinger == "right-ring-finger") {
                    return i18n("Please repeatedly swipe your right ring finger on the fingerprint sensor.")
                } else if (kcm.fingerprintModel.currentFinger == "right-little-finger") {
                    return i18n("Please repeatedly swipe your right little finger on the fingerprint sensor.")
                } else if (kcm.fingerprintModel.currentFinger == "right-thumb") {
                    return i18n("Please repeatedly swipe your right thumb on the fingerprint sensor.")
                } else if (kcm.fingerprintModel.currentFinger == "left-index-finger") {
                    return i18n("Please repeatedly swipe your left index finger on the fingerprint sensor.")
                } else if (kcm.fingerprintModel.currentFinger == "left-middle-finger") {
                    return i18n("Please repeatedly swipe your left middle finger on the fingerprint sensor.")
                } else if (kcm.fingerprintModel.currentFinger == "left-ring-finger") {
                    return i18n("Please repeatedly swipe your left ring finger on the fingerprint sensor.")
                } else if (kcm.fingerprintModel.currentFinger == "left-little-finger") {
                    return i18n("Please repeatedly swipe your left little finger on the fingerprint sensor.")
                } else if (kcm.fingerprintModel.currentFinger == "left-thumb") {
                    return i18n("Please repeatedly swipe your left thumb on the fingerprint sensor.")
                }
            }
            return ""
        }

        Layout.alignment: Qt.AlignHCenter
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
        Layout.maximumWidth: parent.width
        // visible: kcm.fingerprintModel.dialogState === FingerprintDialog.DialogState.Enrolling
    }

    onVisibleChanged: progressCircle.colorTimer.restart();

    // progress circle
    FingerprintProgressCircle {
        id: progressCircle
    }

    QQC2.Label {
        text: kcm.fingerprintModel.enrollFeedback
        wrapMode: Text.Wrap
        Layout.maximumWidth: parent.width
        Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
    }
}
