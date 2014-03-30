/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

Copyright (C) 2011 Aaron Seigo <aseigo@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

PlasmaComponents.Button {
    property string label
    property string normalLabel
    property string accelLabel
    text: parent.showAccel ? accelLabel : normalLabel
    property int accelKey: -1

    onLabelChanged: {
        var i = label.indexOf('&');
        if (i < 0) {
            accelLabel = '<u>' + label[0] + '</u>' + label.substring(1, label.length);
            accelKey = label[0].toUpperCase().charCodeAt(0);
            normalLabel = label
        } else {
            var stringToReplace = label.substr(i, 2);
            accelKey = stringToReplace.toUpperCase().charCodeAt(1);
            accelLabel = label.replace(stringToReplace, '<u>' + stringToReplace[1] + '</u>');
            normalLabel = label.replace(stringToReplace, stringToReplace[1]);
        }
    }
}

