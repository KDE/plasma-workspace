/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Components project.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kquickcontrolsaddons 2.0

Item {
    id: root

    property string text
    property int index: 0
    property bool subMenu: false
    property bool allowAmpersand: false

    signal clicked

    property int implicitWidth: textArea.paintedWidth + 2*11 + subMenuIcon.width + 8
    width: parent.width
    height: textArea.paintedHeight + 8

    onTextChanged: {
        if (allowAmpersand) {
            textArea.text = root.text
        } else {
            textArea.text = root.text.replace('&', '')
        }
    }

    PlasmaComponents.Label {
        id: textArea
        anchors.left: background.left
        anchors.leftMargin: 11
        elide: Text.ElideRight
    }

    PlasmaCore.SvgItem {
        id: background
        anchors.fill: parent

        svg: PlasmaCore.Svg {
            imagePath: "dialogs/shutdowndialog"
        }
        elementId: "button-hover"
        visible: root.ListView.isCurrentItem
    }

    QIconItem {
        id: subMenuIcon

        // if textColor is closer to white than to black use "draw-triangle4", which is also close to white.
        // Otherwise use "arrow-down", which is green. I have not found a black triangle icon.
        icon: theme.textColor > "#7FFFFF" ? QIcon("draw-triangle4") : QIcon("arrow-down")

        width: 6
        height: width
        visible: root.subMenu

        anchors {
            right: background.right
            rightMargin: 11
            verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        id: mouseArea

        property bool canceled: false
        hoverEnabled: true

        anchors.fill: parent

        onPressed: {
            canceled = false
        }
        onClicked: {
            if (!canceled)
                root.clicked()
        }
        onEntered: {
            root.ListView.view.currentIndex = root.ListView.view.indexAt(root.x, root.y)
        }
        onExited: {
            canceled = true
        }
    }

    Keys.onPressed: {
        event.accepted = true
        switch (event.key) {
            case Qt.Key_Select:
            case Qt.Key_Enter:
            case Qt.Key_Return: {
                if (!event.isAutoRepeat) {
                        root.clicked()
                }
                break
            }

            case Qt.Key_Up: {
                    if (ListView.view != null)
                        ListView.view.decrementCurrentIndex()
                    else
                        event.accepted = false
                break
            }

            case Qt.Key_Down: {
                    if (ListView.view != null)
                        ListView.view.incrementCurrentIndex()
                    else
                        event.accepted = false
                break
            }
            default: {
                event.accepted = false
                break
            }
        }
    }
}
