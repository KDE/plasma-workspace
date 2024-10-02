/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2024 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami 2.20 as Kirigami

Kirigami.Icon {
    id: iconItem

    Layout.alignment: Qt.AlignTop

    implicitWidth: Kirigami.Units.iconSizes.large
    implicitHeight: Kirigami.Units.iconSizes.large

    property var applicationIconSource
    // don't show two identical icons
    visible: valid && source != applicationIconSource

    smooth: true
}

