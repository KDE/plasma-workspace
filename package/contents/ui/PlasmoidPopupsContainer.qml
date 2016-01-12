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
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    id: popupsContainer

    Layout.minimumWidth: 300
    Layout.minimumHeight: 300

    property Item activeApplet

    onActiveAppletChanged: {
        if (activeApplet != null) {
            mainStack.replace(activeApplet.fullRepresentationItem);
        } else {
            mainStack.pop();
        }
    }

    PlasmaExtras.Heading {
        id: heading
        level: 1

        anchors {
            top: parent.top
            topMargin: units.gridUnit
            left: parent.left
            leftMargin: units.largeSpacing
            right: parent.right
        }
        text: activeApplet ? activeApplet.title : i18n("Status & Notifications")
    }

    PlasmaComponents.PageStack {
        id: mainStack
        anchors {
            top: heading.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }
}
