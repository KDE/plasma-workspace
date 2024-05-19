/*
    SPDX-FileCopyrightText: 2020 Devin Lin <espidev@gmail.com>
    SPDX-FileCopyrightText: 2024 Nicolas Fella <nicolas.fella@gmx.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami
import FingerprintModel

ColumnLayout {
    id: root

    required property bool done
    required property int scanType
    required property string finger
    required property string enrollFeedback

    spacing: Kirigami.Units.largeSpacing * 2

    QQC2.Label {
        text: {
            if (root.scanType == FprintDevice.Press) {
                if (root.finger == "right-index-finger") {
                    return i18n("Please repeatedly press your right index finger on the fingerprint sensor.");
                } else if (root.finger == "right-middle-finger") {
                    return i18n("Please repeatedly press your right middle finger on the fingerprint sensor.");
                } else if (root.finger == "right-ring-finger") {
                    return i18n("Please repeatedly press your right ring finger on the fingerprint sensor.");
                } else if (root.finger == "right-little-finger") {
                    return i18n("Please repeatedly press your right little finger on the fingerprint sensor.");
                } else if (root.finger == "right-thumb") {
                    return i18n("Please repeatedly press your right thumb on the fingerprint sensor.");
                } else if (root.finger == "left-index-finger") {
                    return i18n("Please repeatedly press your left index finger on the fingerprint sensor.");
                } else if (root.finger == "left-middle-finger") {
                    return i18n("Please repeatedly press your left middle finger on the fingerprint sensor.");
                } else if (root.finger == "left-ring-finger") {
                    return i18n("Please repeatedly press your left ring finger on the fingerprint sensor.");
                } else if (root.finger == "left-little-finger") {
                    return i18n("Please repeatedly press your left little finger on the fingerprint sensor.");
                } else if (root.finger == "left-thumb") {
                    return i18n("Please repeatedly press your left thumb on the fingerprint sensor.");
                }
            } else if (root.scanType == FprintDevice.Swipe) {
                if (root.finger == "right-index-finger") {
                    return i18n("Please repeatedly swipe your right index finger on the fingerprint sensor.");
                } else if (root.finger == "right-middle-finger") {
                    return i18n("Please repeatedly swipe your right middle finger on the fingerprint sensor.");
                } else if (root.finger == "right-ring-finger") {
                    return i18n("Please repeatedly swipe your right ring finger on the fingerprint sensor.");
                } else if (root.finger == "right-little-finger") {
                    return i18n("Please repeatedly swipe your right little finger on the fingerprint sensor.");
                } else if (root.finger == "right-thumb") {
                    return i18n("Please repeatedly swipe your right thumb on the fingerprint sensor.");
                } else if (root.finger == "left-index-finger") {
                    return i18n("Please repeatedly swipe your left index finger on the fingerprint sensor.");
                } else if (root.finger == "left-middle-finger") {
                    return i18n("Please repeatedly swipe your left middle finger on the fingerprint sensor.");
                } else if (root.finger == "left-ring-finger") {
                    return i18n("Please repeatedly swipe your left ring finger on the fingerprint sensor.");
                } else if (root.finger == "left-little-finger") {
                    return i18n("Please repeatedly swipe your left little finger on the fingerprint sensor.");
                } else if (root.finger == "left-thumb") {
                    return i18n("Please repeatedly swipe your left thumb on the fingerprint sensor.");
                }
            }
            return "";
        }
        textFormat: Text.PlainText

        Layout.alignment: Qt.AlignHCenter
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
        Layout.maximumWidth: parent.width
        visible: !root.done
    }

    Kirigami.Heading {
        level: 2
        text: i18n("Finger Enrolled")
        textFormat: Text.PlainText
        Layout.alignment: Qt.AlignHCenter
        visible: root.done
    }

    // reset from back from whatever color was used before
    onVisibleChanged: progressCircle.colorTimer.restart()

    // progress circle
    FingerprintProgressCircle {
        id: progressCircle

        implicitWidth: 80
        implicitHeight: 80

        Layout.alignment: Qt.AlignHCenter
    }

    QQC2.Label {
        text: root.enrollFeedback
        textFormat: Text.PlainText
        wrapMode: Text.Wrap
        Layout.alignment: Qt.AlignHCenter
    }
}
