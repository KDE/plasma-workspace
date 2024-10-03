/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2024 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

GridLayout {
    id: baseDelegate

    Layout.fillWidth: true

    property ModelInterface modelInterface

    property bool menuOpen

    rowSpacing: Kirigami.Units.smallSpacing
    columnSpacing: Kirigami.Units.smallSpacing

    Accessible.role: Accessible.NoRole
    Accessible.name: modelInterface.summary
    Accessible.description: modelInterface.accessibleDescription
    columns: 2

    //TODO: REMOVE
    property real headerHeight: 0
}

