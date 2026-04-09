/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2026 Nate Graham <nate@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import org.kde.kirigami as Kirigami

Kirigami.Badge {

    // TODO Plasma 7: unused, remove
    property Item icon

    customColor: Qt.alpha(Kirigami.Theme.backgroundColor, 0.9)
    padding: 0
    font.bold: false
    font.pixelSize: 9
}
