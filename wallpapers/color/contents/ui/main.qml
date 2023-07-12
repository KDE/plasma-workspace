/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kirigami 2.20 as Kirigami
import org.kde.plasma.plasmoid 2.0

WallpaperItem {
    id: root

    Rectangle {
        anchors.fill: parent
        color: root.configuration.Color

        Behavior on color {
            SequentialAnimation {
                ColorAnimation {
                    duration: Kirigami.Units.longDuration
                }

                ScriptAction {
                    script: root.repaintNeeded()
                }
            }
        }
    }
}
