/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2024 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.ksvg as KSvg

import "../components" as Components

GridLayout {
    id: baseDelegate

    Layout.fillWidth: true

    //property ModelInterface modelInterface
    property Components.ModelInterface modelInterface: Components.ModelInterface {}

    property Item body
    property Item footer
    //property bool menuOpen
    readonly property real textPreferredWidth: Kirigami.Units.gridUnit * 18
    readonly property bool menuOpen: Boolean(body?.item?.menuOpen)
                                     || Boolean(footer?.item?.menuOpen)
    readonly property bool dragging: Boolean(footer?.item?.dragging)
    readonly property bool replying: footer?.item?.replying ?? false
    readonly property bool hasPendingReply: footer?.item?.hasPendingReply ?? false

    rowSpacing: Kirigami.Units.smallSpacing
    columnSpacing: Kirigami.Units.smallSpacing

    Accessible.role: Accessible.NoRole
    Accessible.name: modelInterface.summary
    Accessible.description: modelInterface.accessibleDescription
    columns: 3
}

