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

import org.kde.draganddrop 2.0 as DragAndDrop

// MousePage

PlasmaComponents.Page {
    id: dragPage

    property int _h: 48
    property bool isDragging: false


    anchors {
        fill: parent
        margins: _s
    }

    PlasmaExtras.Title {
        id: dlabel

        anchors { left: parent.left; right: parent.right; top: parent.top; }

        text: "Drag & Drop"
    }

    Column {
        id: dragCol

        width: (parent.width-_h)/2
        height: parent.height
        anchors {
            left: parent.left;
            top: parent.top;
            topMargin: itemHeight/2

        }

        spacing: _h/4


        property int itemHeight: _h*1.5

        DragItem {
            text: "Image and URL"
            icon: "image-png"
            height: parent.itemHeight
            DragAndDrop.DragArea {
                objectName: "imageandurl"
                anchors { fill: parent; }
                //delegateImage: "akonadi"
                mimeData.url: "http://plasma.kde.org"
                onDragStarted: {
                    isDragging = true;
                    print(" drag started for " + objectName);
                    ooo.text = objectName
                }
                onDrop: {
                    isDragging = false;
                    print(" item dropped " + objectName);
                    ooo.text = objectName
                }
                //Rectangle { anchors.fill: parent; color: "blue"; opacity: 0.4; }
            }
        }
        DragItem {
            text: "Delegate Image"
            icon: "image-png"
            height: parent.itemHeight
            DragAndDrop.DragArea {
                objectName: "image"
                anchors { fill: parent; }
                //delegateImage: "akonadi"
                //mimeData.url: "http://plasma.kde.org"
                onDragStarted: {
                    isDragging = true;
                    print(" drag started for " + objectName);
                    ooo.text = objectName
                }
                onDrop: {
                    isDragging = false;
                    print(" item dropped " + objectName);
                    ooo.text = objectName
                }
                //Rectangle { anchors.fill: parent; color: "green"; opacity: 0.4; }
            }
        }
        DragItem {
            text: "HTML"
            icon: "text-html"
            height: parent.itemHeight
            DragAndDrop.DragArea {
                objectName: "html"
                anchors { fill: parent; }
                mimeData.html: "<b>One <i> Two <u> Three </b> Four </i>Five </u> "
                onDragStarted: {
                    isDragging = true;
                    print(" drag started for " + objectName);
                    ooo.text = objectName
                }
                onDrop: {
                    isDragging = false;
                    print(" item dropped " + objectName);
                    ooo.text = objectName
                }
            }
        }
        DragItem {
            text: "Color"
            icon: "preferences-color"
            height: parent.itemHeight
            DragAndDrop.DragArea {
                objectName: "color"
                anchors { fill: parent; }
                mimeData.color: "orange"
                onDragStarted: {
                    isDragging = true;
                    print(" drag started for " + objectName);
                    ooo.text = objectName
                }
                onDrop: {
                    isDragging = false;
                    print(" item dropped " + objectName);
                    ooo.text = objectName
                    //mimeData.
                }
            }
        }
        DragItem {
            text: "Lots of Stuff"
            icon: "ksplash"
            height: parent.itemHeight
            DragAndDrop.DragArea {
                id: dragArea2
                objectName: "stuff"
//                 width: parent.width / 2
//                 height: dropArea.height / 2
                anchors.fill: parent

                mimeData.text: "Clownfish"
                mimeData.html: "<h2>Swimming in a Sea of Cheese</h2><pre>Primus->perform();</pre><br/>"
                mimeData.color: "darkred"
                mimeData.url: "http://plasma.kde.org"
                mimeData.urls: ["http://planetkde.org", "http://fsfe.org", "http://techbase.kde.org", "http://qt-project.org"]

                //Rectangle { anchors.fill: parent; color: "yellow"; opacity: 0.6; }

                onDragStarted: {
                    isDragging = true;
                    print(" drag started for " + objectName);
                    ooo.text = objectName
                }
                onDrop: {
                    isDragging = false;
                    print(" item dropped " + objectName);
                    ooo.text = objectName
                }
            }
        }
        PlasmaComponents.Label {
            id: ooo
        }
    }
    DragAndDrop.DropArea {
        id: dropArea
        //width: parent.width- / 2
        //visible: false
        anchors {
            right: parent.right;
            left: dragCol.right;
            bottom: parent.bottom;
            top: parent.top; margins: _h/2

        }

        PlasmaComponents.ListItem {
            id: dropHightlight
            anchors.fill: parent
            opacity: 0

            PropertyAnimation { properties: "opacity"; easing.type: Easing.Linear; duration: 2000; }
        }

        Rectangle { id: clr; anchors.fill: parent; color: "transparent"; opacity: color != "transparent" ? 1 : 0; }

        PlasmaComponents.Label {
            id: ilabel
            font.pointSize: _h / 2
            text: "Drop here."
            opacity: isDragging ? 0.7 : 0
            anchors.centerIn: parent
            horizontalAlignment: Text.AlignCenter
            PropertyAnimation { properties: "opacity"; easing.type: Easing.Linear; duration: 2000; }
        }

        PlasmaComponents.Label {
            id: slabel
            font.pointSize: _h / 4
            //text: "Drop here."
            //opacity: isDragging ? 1 : 0
            //onTextChanged: print("droparea changed to " + text)
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.left: parent.left
            //horizontalAlignment: Text.AlignCenter
        }

        onDragEnter: {
//             slabel.text = "drop item here";
            dropHightlight.opacity = 1;
        }
        onDragLeave: {
//             slabel.text = "drop left";
            dropHightlight.opacity = 0;
        }
        onDrop: {
            var txt = event.mimeData.html;
            txt += event.mimeData.text;
            if (event.mimeData.url != "") {
                txt += "<br />Url: " + event.mimeData.url;
            }
            var i = 0;
            var u;
            for (u in event.mimeData.urls) {
                txt += "<br />  Url " + i + " : " + event.mimeData.urls[i];
                i++;
            }
//             print("COLOR: " + event.mimeData.color);
            if (event.mimeData.hasColor()) {
                clr.color = event.mimeData.color;
            } else {
                clr.color = "transparent";
            }
            slabel.text = txt
            dropHightlight.opacity = 0.5;
        }
    }
}

