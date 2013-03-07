/*
 *  Copyright 2013 Sebastian Kügler <sebas@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0

import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.plasma.extras 0.1 as PlasmaExtras
import org.kde.qtextracomponents 0.1 as QtExtras

// PlasmoidPage

PlasmaComponents.Page {
    id: plasmoidPage
    anchors {
        fill: parent
        margins: _s
    }
    Column {
        anchors.centerIn: parent
        spacing: _s
        PlasmaExtras.Heading {
            level: 2
            text: "I'm an applet"
        }

        PlasmaComponents.Button {
            height: theme.iconSizes.desktop
            text: "Background"
            checked: plasmoid.backgroundHints == 1
            onClicked: {
                print("Background hints: " + plasmoid.backgroundHints)
                if (plasmoid.backgroundHints == 0) {
                    plasmoid.backgroundHints = 1//TODO: make work "StandardBackground"
                } else {
                    plasmoid.backgroundHints = 0//TODO: make work "NoBackground"
                }
            }
        }
        PlasmaComponents.Button {
            height: theme.iconSizes.desktop
            text: "Busy"
            checked: plasmoid.busy
            onClicked: {
                plasmoid.busy = !plasmoid.busy
            }
        }
    }
}

