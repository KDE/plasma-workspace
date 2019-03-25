/*
 * Copyright 2014 Marco Martin <mart@kde.org>
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

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.plasmoid 2.0
import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons

Item {
    id: root

    z: 9999
    property bool horizontal: plasmoid.formFactor !== PlasmaCore.Types.Vertical

    Layout.fillWidth: plasmoid.configuration.expanding
    Layout.fillHeight: plasmoid.configuration.expanding

    Layout.minimumWidth: 1
    Layout.minimumHeight: 1
    Layout.preferredWidth: horizontal ? plasmoid.configuration.length : 0
    Layout.preferredHeight: horizontal ? 0 : plasmoid.configuration.length

    Plasmoid.preferredRepresentation: Plasmoid.fullRepresentation

    function action_expanding() {
        plasmoid.configuration.expanding = plasmoid.action("expanding").checked;
    }
    Component.onCompleted: {
        plasmoid.setAction("expanding", i18n("Set flexible size"));
        var action = plasmoid.action("expanding");
        action.checkable = true;
        action.checked = plasmoid.configuration.expanding;

        plasmoid.removeAction("configure");
    }
}
