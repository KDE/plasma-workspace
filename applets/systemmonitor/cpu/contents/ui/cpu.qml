/*
 *   Copyright 2015 Marco Martin <mart@kde.org>
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kquickcontrolsaddons 2.0 as KQuickAddons

Applet {
    id: root

    onSourceAdded: {
        var match = source.match(/^cpu\/(\w+)\/TotalLoad/);
        if (match) {
            root.addSource(source, match[1]);
        }
    }

    delegate: SinglePlotter {
        autoRange: false
        rangeMin: 0
        rangeMax: 100
        function formatLabel(data) {
            //i18nc("CPU usage: %1 is the value, %2 the unit datatype", "%1 %2")
            //return i18n("%1 %2", Math.round(data.value), data.units);
            return i18nc("CPU usage: %1 is the value, %2 the unit datatype", "%1 %2", Math.round(data.value), data.units);
        }
    }
}
