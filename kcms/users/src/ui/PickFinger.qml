/*
    SPDX-FileCopyrightText: 2020 Devin Lin <espidev@gmail.com>
    SPDX-FileCopyrightText: 2024 Nicolas Fella <nicolas.fella@gmx.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import FingerprintModel
import org.kde.plasma.kcm.users as UsersKCM

ColumnLayout {
    id: root

    required property var availableFingers
    required property var unavailableFingers

    spacing: Kirigami.Units.largeSpacing

    signal fingerPicked(string finger)

    Kirigami.Heading {
        level: 2
        text: i18n("Pick a finger to enroll")
        textFormat: Text.PlainText
        Layout.alignment: Qt.AlignHCenter
    }

    Item {
        id: handContainer
        implicitHeight: basePalm.implicitHeight
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
            model: root.availableFingers
            delegate: Image {
                id: img

                required property string internalName
                required property string friendlyName

                activeFocusOnTab: true
                source: kcm.recolorSVG(Qt.resolvedUrl(`hand-images/${internalName}.svg`), color)
                readonly property color color: focus ? Kirigami.Theme.focusColor : (maskArea.hovered ? Kirigami.Theme.hoverColor : Kirigami.Theme.textColor)

                fillMode: Image.PreserveAspectFit
                anchors.fill: parent
                Accessible.name: img.friendlyName
                Accessible.focusable: true
                Accessible.role: Accessible.RadioButton
                Accessible.onPressAction: {
                    img.activate();
                }
                Keys.onEnterPressed: {
                    img.activate();
                }
                function activate() {
                    root.fingerPicked(img.internalName);
                }
                UsersKCM.MaskMouseArea {
                    id: maskArea
                    anchors.fill: parent
                    onTapped: {
                        img.activate();
                    }
                }
            }
        }

        Repeater {
            model: root.unavailableFingers
            delegate: Image {

                required property string internalName

                source: kcm.recolorSVG(Qt.resolvedUrl(`hand-images/${internalName}.svg`), Kirigami.Theme.textColor)
                fillMode: Image.PreserveAspectFit
                anchors.fill: parent
                opacity: 0.25
            }
        }
    }
}
