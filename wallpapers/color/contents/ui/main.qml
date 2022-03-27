/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore

Rectangle {
    id: root
    color: randomColorGeneratorLoader.active ? randomColorGeneratorLoader.item.randomColor
                                             : wallpaper.configuration.Color

    function action_next() {
        randomColorGeneratorLoader.item.restart();
    }

    Behavior on color {
        ColorAnimation { duration: PlasmaCore.Units.longDuration }
    }

    Loader {
        id: randomColorGeneratorLoader
        active: wallpaper.configuration.UseRandomColors
        source: "RandomColorGenerator.qml"

        onLoaded: wallpaper.setAction("next", i18nc("@action:inmenu switch to next random color", "Next Random Color"), "color-fill")
        onActiveChanged: if (!active) {
            wallpaper.removeAction("next");
        }
    }
}
