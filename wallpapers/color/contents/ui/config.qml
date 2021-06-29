/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0
import org.kde.kquickcontrols 2.0 as KQuickControls
import org.kde.kirigami 2.5 as Kirigami

Kirigami.FormLayout {
    id: root
    twinFormLayouts: parentLayout

    property alias cfg_Color: colorButton.color
    property alias formLayout: root

    KQuickControls.ColorButton {
        id: colorButton
        Kirigami.FormData.label: i18nd("plasma_wallpaper_org.kde.color", "Color:")
        dialogTitle: i18nd("plasma_wallpaper_org.kde.color", "Select Background Color")
    }
}
