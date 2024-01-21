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

ListView {
    id: fingerprintsList

    model: kcm.fingerprintModel.deviceFound ? kcm.fingerprintModel.enrolledFingerprints : 0

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - (Kirigami.Units.largeSpacing * 4)
        visible: fingerprintsList.count == 0
        text: i18n("No fingerprints added")
        icon.name: "fingerprint"
    }

    delegate: QQC2.ItemDelegate {
        id: delegate

        text: modelData.friendlyName
        width: ListView.view.width
        hoverEnabled: false
        down: false
        highlighted: false

        contentItem: RowLayout {
            Kirigami.IconTitleSubtitle {
                title: delegate.text
                icon.name: "fingerprint"

                Layout.fillWidth: true
            }

            QQC2.ToolButton {
                icon.name: "edit-entry"
                text: i18n("Re-enroll finger")
                display: QQC2.Button.IconOnly

                onClicked: {
                    kcm.fingerprintModel.currentFinger = modelData.internalName
                    kcm.fingerprintModel.startEnrolling(modelData.internalName)
                }
            }

            QQC2.ToolButton {
                icon.name: "edit-delete"
                text: i18n("Delete fingerprint")
                display: QQC2.Button.IconOnly

                onClicked: {
                    kcm.fingerprintModel.deleteFingerprint(modelData.internalName)
                }
            }
        }
    }
}
