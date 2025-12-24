/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2024 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

Kirigami.Icon {
    id: iconItem
    Layout.alignment: Qt.AlignTop

    property ModelInterface modelInterface
    readonly property bool dragging: (jobIconLoader.item as JobIconItem)?.dragging ?? false

    implicitWidth: Kirigami.Units.iconSizes.large
    implicitHeight: Kirigami.Units.iconSizes.large

    source: modelInterface.icon
    // don't show two identical icons
    visible: valid || ((jobIconLoader.item as JobIconItem)?.shown ?? false)

    smooth: true

    Loader {
        id: jobIconLoader
        anchors.fill: parent
        active: iconItem.modelInterface.jobDetails?.effectiveDestUrl ?? false
        sourceComponent: JobIconItem {
            modelInterface: iconItem.modelInterface
        }
    }
}

