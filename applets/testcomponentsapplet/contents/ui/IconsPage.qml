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

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.qtextracomponents 2.0 as QtExtras

// IconTab

PlasmaComponents.Page {
    id: iconsPage
    anchors {
        fill: parent
        margins: _s
    }
    Column {
        anchors.fill: parent
        spacing: _s

        PlasmaExtras.Title {
            width: parent.width
            elide: Text.ElideRight
            text: "Icons"
        }
        Row {
            height: _h
            spacing: _s

            PlasmaCore.IconItem {
                source: "configure"
                width: parent.height
                height: width
            }
            PlasmaCore.IconItem {
                source: "dialog-ok"
                width: parent.height
                height: width
            }

            PlasmaCore.IconItem {
                source: "maximize"
                width: parent.height
                height: width
            }


            PlasmaCore.IconItem {
                source: "akonadi"
                width: parent.height
                height: width
            }
            PlasmaCore.IconItem {
                source: "clock"
                width: parent.height
                height: width
            }
            QtExtras.QIconItem {
                icon: "preferences-desktop-icons"
                width: parent.height
                height: width
            }

        }

        PlasmaComponents.Label {
            text: "iconSizes.small  : " + theme.iconSizes.small +
                    ", iconSizes.desktop: " + theme.iconSizes.desktop +
                    ",<br />iconSizes.toolbar: " + theme.iconSizes.toolbar +
                    ", iconSizes.dialog : " + theme.iconSizes.dialog

        }
    }
}
