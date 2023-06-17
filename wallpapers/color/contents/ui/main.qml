/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0
import org.kde.kirigami 2.20 as Kirigami

Rectangle {
    id: root
    color: wallpaper.configuration.Color

    Behavior on color {
        SequentialAnimation {
            ColorAnimation {
                duration: Kirigami.Units.longDuration
            }

            ScriptAction {
                script: wallpaper.repaintNeeded()
            }
        }
    }
}
