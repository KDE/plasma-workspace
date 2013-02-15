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

Item {
    id: root
    width: 100
    height: 100

    property int _s: 12
    property int _h: 32

    PlasmaComponents.TabBar {
        id: tabBar

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        height: _h

        currentTab: melPage
        PlasmaComponents.TabButton { tab: pageOne; iconSource: "preferences-desktop-icons"}
        PlasmaComponents.TabButton { tab: pageTwo; iconSource: "plasma"}
        PlasmaComponents.TabButton { tab: melPage; iconSource: "preferences-desktop-mouse"}
    }

    PlasmaComponents.TabGroup {
        anchors {
            left: parent.left
            right: parent.right
            top: tabBar.bottom
            bottom: parent.bottom
        }

        //currentTab: tabBar.currentTab

        PlasmaComponents.Page {
            id: pageOne
            anchors {
                fill: parent
                margins: _s
            }
            Column {
                anchors.fill: parent
                spacing: _s

                PlasmaExtras.Title {
                    width: parent.width
                    text: "This is a <i>PlasmaComponent</i>"
                }
                PlasmaComponents.Label {
                    width: parent.width
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
                PlasmaExtras.Heading {
                    level: 4
                    width: parent.width
                    text: "Buttons"
                }
                Column {
                    width: parent.width
                    spacing: _s

                    PlasmaComponents.Button {
                        text: "Button"
                        iconSource: "call-start"
                    }
                    PlasmaComponents.ToolButton {
                        text: "ToolButton"
                        iconSource: "call-stop"
                    }
                    PlasmaComponents.RadioButton {
                        text: "RadioButton"
                        //iconSource: "call-stop"
                    }
                }

            }
        }

        PlasmaComponents.Page {
            id: pageTwo
            Column {
                anchors.centerIn: parent
                PlasmaExtras.Heading {
                    level: 2
                    text: "I'm an applet"
                }
                PlasmaComponents.Button {
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
                    text: "Busy"
                    checked: plasmoid.busy
                    onClicked: {
                        plasmoid.busy = !plasmoid.busy
                    }
                }
            }
        }

        PlasmaComponents.Page {
            id: melPage
            PlasmaExtras.Title {
                id: mellabel
                text: "MouseEventListener"
                anchors { left: parent.left; right: parent.right; top: parent.top }
            }
            QtExtras.MouseEventListener {
                id: mel
                hoverEnabled: true
                anchors { left: parent.left; right: parent.right; top: mellabel.bottom; bottom: parent.bottom; }
                /*
                void pressed(KDeclarativeMouseEvent *mouse);
                void positionChanged(KDeclarativeMouseEvent *mouse);
                void released(KDeclarativeMouseEvent *mouse);
                void clicked(KDeclarativeMouseEvent *mouse);
                void pressAndHold(KDeclarativeMouseEvent *mouse);
                void wheelMoved(KDeclarativeWheelEvent *wheel);
                void containsMouseChanged(bool containsMouseChanged);
                void hoverEnabledChanged(bool hoverEnabled);
                */
                onPressed: {
                    melstatus.text = "pressed";
                }
                onPositionChanged: {
                    print("positionChanged: " + mouse.x + "," + mouse.y);
                }
                onReleased: {
                    print("Released");
                    melstatus.text = "Released";
                }
                onPressAndHold: {
                    print("pressAndHold");
                    melstatus.text = "pressAndHold";
                }
                onClicked: {
                    print("Clicked");
                    melstatus.text = "clicked";
                }
                onWheelMoved: {
                    print("Wheel: " + wheel.delta);
                }
                onContainsMouseChanged: {
                    print("Contains mouse: " + containsMouse);
                }

                MouseArea {
                    //target: mel
                    anchors.fill: parent
                    onPressed: PlasmaExtras.DisappearAnimation { targetItem: bgImage }
                    onReleased: PlasmaExtras.AppearAnimation { targetItem: bgImage }
                }
                Image {
                    id: bgImage
                    source: "image://appbackgrounds/standard"
                    fillMode: Image.Tile
                    anchors.fill: parent
                    asynchronous: true
    //                 opacity: mel.containsMouse ? 1 : 0.2
    //                 Behavior on opacity { PropertyAnimation {} }
                }
                Column {
                    //width: parent.width
                    spacing: _s
                    anchors.fill: parent
                    PlasmaComponents.Button {
                        text: "Button"
                        iconSource: "call-start"
                    }
                    PlasmaComponents.ToolButton {
                        text: "ToolButton"
                        iconSource: "call-stop"
                    }
                    PlasmaComponents.RadioButton {
                        text: "RadioButton"
                        //iconSource: "call-stop"
                    }
                    PlasmaComponents.Label {
                        id: melstatus
                    }
                }

            }
        }

    }

    Component.onCompleted: {
        print("Components Test Applet loaded")
    }
}