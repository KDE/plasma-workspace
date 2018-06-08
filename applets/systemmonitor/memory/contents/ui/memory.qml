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
import org.kde.kquickcontrolsaddons 2.0 as KQuickAddons
import org.kde.kcoreaddons 1.0 as KCoreAddons

Applet {
    id: root

    onSourceAdded: {
        if (source === "mem/physical/application") {
            root.addSource(source, i18n("Physical memory"));
        } else if (source === "mem/swap/used") {
            root.addSource(source, i18n("Swap"));
        }
    }

    delegate: SinglePlotter {
        autoRange: false
        rangeMin: 0
        rangeMax: 0
        function formatData(data) {
            rangeMax = data.max;
            return KCoreAddons.Format.formatByteSize(data.value * 1024)
        }
    }
}
