/***************************************************************************
 *   Copyright (C) 2016 Marco Martin <mart@kde.org>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

import QtQuick 2.2
import QtQuick.Layouts 1.2

import org.kde.plasma.core 2.0 as PlasmaCore

import "../components"

ActionButton {
    property var action
    onClicked: action()
    iconSize: units.iconSizes.huge
    opacity: activeFocus || containsMouse ? 1 : 0.6
    Behavior on opacity {
        OpacityAnimator {
            duration: units.longDuration
            easing.type: Easing.InOutQuad
        }
    }
}
