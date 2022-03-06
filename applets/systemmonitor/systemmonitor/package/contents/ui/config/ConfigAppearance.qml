/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2019 Arjen Hiemstra <ahiemstra@heimr.nl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.9
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2 as QQC2

import org.kde.kirigami 2.5 as Kirigami
import org.kde.kquickcontrols 2.0
import org.kde.kconfig 1.0 // for KAuthorized
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.newstuff 1.62 as NewStuff

import org.kde.ksysguard.sensors 1.0 as Sensors
import org.kde.ksysguard.faces 1.0 as Faces

QQC2.Control {
    id: root

    signal configurationChanged

    function saveConfig() {
        contentItem.saveConfig();
        Plasmoid.nativeInterface.faceController.reloadConfig()
    }

    // Workaround for Bug 424458, when reusing the controller/item things break
    contentItem: Plasmoid.nativeInterface.workaroundController(root).appearanceConfigUi

    Connections {
        target: contentItem
        function onConfigurationChanged() {
            root.configurationChanged()
        }
    }
}
