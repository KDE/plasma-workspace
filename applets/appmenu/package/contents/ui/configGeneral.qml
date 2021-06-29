/*
    SPDX-FileCopyrightText: 2016 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.0
import QtQuick.Controls 2.5

import org.kde.kirigami 2.5 as Kirigami
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore

Kirigami.FormLayout {
    anchors.left: parent.left
    anchors.right: parent.right
    
    property alias cfg_compactView: compactViewRadioButton.checked

    RadioButton {
        id: compactViewRadioButton
        text: i18n("Use single button for application menu")
    }

    RadioButton {
        id: fullViewRadioButton
        checked: !compactViewRadioButton.checked
        text: i18n("Show full application menu")
    }
}
