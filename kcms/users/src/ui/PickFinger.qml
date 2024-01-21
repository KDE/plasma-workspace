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
import QtQuick.Layouts 1.3
import QtQuick.Shapes 1.12
import QtQuick.Controls 2.5 as QQC2

import org.kde.kirigami 2.12 as Kirigami
import org.kde.plasma.kcm.users 1.0 as UsersKCM
import FingerprintModel 1.0

ColumnLayout {
    id: pickFinger
    spacing: Kirigami.Units.largeSpacing

    Kirigami.Heading {
        level: 2
        text: i18n("Pick a finger to enroll")
        Layout.alignment: Qt.AlignHCenter
    }

    Item {
        id: handContainer
        implicitHeight: basePalm.height
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignCenter

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
            model: kcm.fingerprintModel.availableFingersToEnroll
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
                    kcm.fingerprintModel.currentFinger = modelData.internalName;
                    kcm.fingerprintModel.startEnrolling(modelData.internalName);
                }
                UsersKCM.MaskMouseArea {
                    id: maskArea
                    anchors.fill: parent
                    onTapped: {
                        img.activate()
                    }
                }
            }
        }

        Repeater {
            model: kcm.fingerprintModel.unavailableFingersToEnroll
            delegate: Image {
                source: kcm.recolorSVG(Qt.resolvedUrl(`hand-images/${modelData.internalName}.svg`), Kirigami.Theme.textColor)
                fillMode: Image.PreserveAspectFit
                anchors.fill: parent
                opacity: 0.25
            }
        }
    }
}
