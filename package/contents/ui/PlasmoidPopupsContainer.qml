/*
 *   Copyright 2015 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.4
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    id: popupsContainer

    Layout.minimumWidth: units.gridUnit * 12
    Layout.minimumHeight: units.gridUnit * 12

    property Item activeApplet

    onActiveAppletChanged: {
        if (activeApplet != null) {
            //reset any potential anchor
            activeApplet.fullRepresentationItem.anchors.left = undefined;
            activeApplet.fullRepresentationItem.anchors.top = undefined;
            activeApplet.fullRepresentationItem.anchors.right = undefined;
            activeApplet.fullRepresentationItem.anchors.bottom = undefined;
            activeApplet.fullRepresentationItem.anchors.centerIn = undefined;
            activeApplet.fullRepresentationItem.anchors.fill = undefined;

            mainStack.replace(activeApplet.fullRepresentationItem);
        } else {
            mainStack.replace(emptyPage);
        }
    }

    PlasmaExtras.Heading {
        id: heading
        level: 1

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        text: activeApplet ? activeApplet.title : i18n("Status & Notifications")
        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (activeApplet) {
                    activeApplet.expanded = false;
                }
            }
        }
    }

    //used to anymate away to nothing
    Item {
        id: emptyPage
    }

    StackView {
        id: mainStack
        //visible: popupsContainer.activeApplet != null
        clip: true
        anchors {
            top: heading.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }
}
