/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
 *  Copyright 2014 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
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
