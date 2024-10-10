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
        configUi.saveConfig();
    }

    readonly property Item configUi: Plasmoid.faceController.faceConfigUi

    // We cannot directly override the contentItem since SimpleKCM is a
    // Kirigami.ScrollablePage which breaks if we override the contentItem. So
    // instead use a placeholder item and reparent the config UI into that item,
    // making sure to bind the required properties so sizing is correct.
    Item {
        id: contents

        implicitWidth: root.configUi.implicitWidth
        implicitHeight: root.configUi.implicitHeight

        children: root.configUi

        Binding {
            target: root.configUi
            property: "width"
            value: contents.width
        }
    }

    Connections {
        target: root.configUi
        function onConfigurationChanged() {
            root.configurationChanged()
        }
    }
}
