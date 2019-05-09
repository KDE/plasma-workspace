/*
 * Copyright 2019 Kai Uwe Broulik <kde@privat.broulik.de>
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

import QtQuick 2.8

import org.kde.plasma.components 2.0 as PlasmaComponents

Item {
    id: labelContainer

    property alias font: label.font
    property alias color: label.color
    property alias text: label.text

    implicitWidth: metrics.tightBoundingRect.width
    implicitHeight: metrics.tightBoundingRect.height

    TextMetrics {
        id: metrics
        font: label.font
        text: label.text
        elide: Qt.ElideNone
    }

    PlasmaComponents.Label {
        id: label
        //x: -metrics.tightBoundingRect.x
        // FIXME why is this completely off?!
        //y: -metrics.tightBoundingRect.y
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        width: metrics.tightBoundingRect.width
        height: metrics.tightBoundingRect.height
    }
}
