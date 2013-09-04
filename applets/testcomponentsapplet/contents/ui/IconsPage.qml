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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
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
        PlasmaComponents.Label {
            text: "iconSizes.small  : " + theme.iconSizes.small +
                    ", iconSizes.desktop: " + theme.iconSizes.desktop +
                    ",<br />iconSizes.toolbar: " + theme.iconSizes.toolbar +
                    ", iconSizes.dialog : " + theme.iconSizes.dialog

        }
        Flow {
            //height: _h
            width: parent.width
            spacing: _s

            PlasmaCore.IconItem {
                source: "configure"
                width: _h
                height: width
            }
            PlasmaCore.IconItem {
                source: "dialog-ok"
                width: _h
                height: width
            }
            PlasmaCore.IconItem {
                source: "resize-tr2bl"
                width: _h
                height: width
            }
            PlasmaCore.IconItem {
                source: "akonadi"
                width: _h
                height: width
            }
            PlasmaCore.IconItem {
                source: "clock"
                width: _h
                height: width
            }
            QtExtras.QIconItem {
                icon: "preferences-desktop-icons"
                width: _h
                height: width
            }

        }
        PlasmaExtras.Heading {
            level: 2
            text: "ToolTip"
        }

        Row {
            spacing: _s
            PlasmaCore.IconItem {
                id: akonadiIcon
                objectName: "akonadiIcon"
                source: "akonadi"
                width: height
                height: _h*2
                //anchors.horizontalCenter: parent.horizontalCenter
                Rectangle { color: "orange"; opacity: 0.3; anchors.fill: parent; }
                PlasmaCore.ToolTip {
                    anchors.fill: parent
                    target: akonadiIcon
                    iconSource: "klipper"
                    mainText: "Fish sighted in the wild, in the wild, a fish was seen."
                    subText: "A mean-looking grouper swam by."
                }
            }
            Image {
                objectName: "bridgeimage"
                height: _h*2
                width: height
                fillMode: Image.PreserveAspectFit
                source: "../images/bridge.jpg"
                PlasmaCore.ToolTip {
                    anchors.fill: parent
                    image: parent.source
                    mainText: "Bridge"
                    subText: "Waalbrug."
                }
            }
            Image {
                objectName: "surfboardimage"
                height: _h*2
                width: height
                fillMode: Image.PreserveAspectFit
                source: "../images/surfboard.jpg"
                PlasmaCore.ToolTip {
                    anchors.fill: parent
                    image: parent.source
                    mainComponent: Component {
                        PlasmaComponents.Label {
                            text: "Nijmegen North Beach"
                            anchors.centerIn: parent
                        }
                    }
                    //subText: "A surfboard on the beach. <br />The photo shows the Waal river's north beach, \
                    //across the water from Nijmegen, Netherlands. It was taken during the summer festivals a few years back."
                }
            }
        }
    }
}
