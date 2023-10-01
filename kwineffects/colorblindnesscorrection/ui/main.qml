/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC
import org.kde.kirigami as Kirigami

import org.kde.kcmutils as KCM
import org.kde.plasma.kwin.colorblindnesscorrectioneffect.kcm

KCM.SimpleKCM {
    id: root

    implicitWidth: Kirigami.Units.gridUnit * 5
    implicitHeight: Kirigami.Units.gridUnit * 5

    Kirigami.FormLayout {
        id: formLayout

        QQC.ComboBox {
            id: showSecondHandCheckBox
            Kirigami.FormData.label: i18nc("@label", "Mode:")
            currentIndex: kcm.settings.mode
            textRole: "text"
            valueRole: "value"
            model: [
                { value: 0, text: i18nc("@option", "Protanopia") },
                { value: 1, text: i18nc("@option", "Deuteranopia") },
                { value: 2, text: i18nc("@option", "Tritanopia") },
            ]

            onActivated: kcm.settings.mode = currentValue
        }
    }
}
