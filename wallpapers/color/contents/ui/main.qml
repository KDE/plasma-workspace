/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid

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
                    script: root.accentColorChanged()
                }
            }
        }
    }
}
