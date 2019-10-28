/*
 * Copyright 2019 Marco Martin <mart@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

import QtQuick 2.10
import org.kde.kirigami 2.11 as Kirigami

MouseArea {
    id: delegate

    property Item contentItem
    property bool draggable: false
    signal dismissRequested

    implicitWidth: contentItem ? contentItem.implicitWidth : 0
    implicitHeight: contentItem ? contentItem.implicitHeight : 0
    opacity: 1 - Math.min(1, 1.5 * Math.abs(x) / width)

    drag {
        axis: Drag.XAxis
        target: draggable && Kirigami.Settings.tabletMode ? this : null
    }

    onReleased: {
        if (Math.abs(x) > width / 2) {
            delegate.dismissRequested();
        } else {
            slideAnim.restart();
        }
    }

    NumberAnimation {
        id: slideAnim
        target: delegate
        property:"x"
        to: 0
        duration: units.longDuration
    }
}
