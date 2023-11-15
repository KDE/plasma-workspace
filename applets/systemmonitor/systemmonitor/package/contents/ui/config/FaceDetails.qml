/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2019 Arjen Hiemstra <ahiemstra@heimr.nl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.9
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2 as QQC2

import org.kde.kquickcontrols 2.0
import org.kde.plasma.plasmoid 2.0
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    id: root

    signal configurationChanged

    function saveConfig() {
        contentItem.saveConfig();
    }

    contentItem: Plasmoid.faceController.faceConfigUi

    Connections {
        target: contentItem
        function onConfigurationChanged() {
            root.configurationChanged()
        }
    }
}
