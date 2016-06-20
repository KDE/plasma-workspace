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
        var match = source.match(/^disk\/([^\/]+)\/Rate\/wblk/);
        if (match) {
            var rSource = "disk/" + match[1] + "/Rate/rblk"
            root.addSource(source, match[1], rSource, match[1]);
        }
    }

    delegate: DoublePlotter {
        function formatLabel(data1, data2) {
            return i18nc("%1 and %2 are values of the same datatype", "%1/s | %2/s", KCoreAddons.Format.formatByteSize(data1.value * 1024),
                        KCoreAddons.Format.formatByteSize(data2.value * 1024));
        }
    }
}
