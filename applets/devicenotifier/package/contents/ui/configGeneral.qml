/*
 *  Copyright 2013 Bhushan Shah <bhush94@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
 */

import QtQuick 2.0
import QtQuick.Controls 1.1 as Controls
import QtQuick.Layouts 1.1 as Layouts

import org.kde.plasma.core 2.0 as PlasmaCore


Item {
    id: iconsPage
    width: childrenRect.width
    height: childrenRect.height
    implicitWidth: mainColumn.implicitWidth
    implicitHeight: mainColumn.implicitHeight

    property alias cfg_removableDevices: removableOnly.checked
    property alias cfg_nonRemovableDevices: nonRemovableOnly.checked
    property alias cfg_allDevices: allDevices.checked
    property alias cfg_popupOnNewDevice: autoPopup.checked

    Layouts.ColumnLayout {
        id: mainColumn
        anchors.left: parent.left
        Controls.ExclusiveGroup{
            id: deviceFilter
        }
        Controls.RadioButton {
            id: removableOnly
            text: i18n("Removable devices only")
            exclusiveGroup: deviceFilter
        }
        Controls.RadioButton {
            id: nonRemovableOnly
            text: i18n("Non-removable devices only")
            exclusiveGroup: deviceFilter
        }
        Controls.RadioButton {
            id: allDevices
            text: i18n("All devices")
            exclusiveGroup: deviceFilter
        }

        Controls.CheckBox {
            id: autoPopup
            Layouts.Layout.fillWidth: true
            text: i18n("Open popup when new device is plugged in")
        }
    }
}
