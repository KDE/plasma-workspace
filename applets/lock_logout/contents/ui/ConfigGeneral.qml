/*
 *  Copyright 2013 Sebastian Kügler <sebas@kde.org>
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
import QtQuick.Controls 1.0 as QtControls

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.private.systemtray 2.0 as SystemTray

Item {
    id: iconsPage
    width: childrenRect.width
    height: childrenRect.height
    implicitWidth: pageColumn.implicitWidth
    implicitHeight: pageColumn.implicitHeight


    property alias cfg_show_requestShutDown: leave.checked
    property alias cfg_show_lockScreen: lock.checked
    property alias cfg_show_switchUser: switchUser.checked
    property alias cfg_show_suspendToDisk: hibernate.checked
    property alias cfg_show_suspendToRam: sleep.checked

    SystemTray.Host {
        id: host
    }

    Column {
        id: pageColumn
        spacing: itemSizeLabel.height / 2
        PlasmaExtras.Title {
            text: i18n("Lock/Logout Settings")
        }

        Column {
            QtControls.CheckBox {
                id: leave
                text: i18n("Leave")
            }
            QtControls.CheckBox {
                id: lock
                text: i18n("Lock")
            }
            QtControls.CheckBox {
                id: switchUser
                text: i18n("Switch User")
            }
            QtControls.CheckBox {
                id: hibernate
                text: i18n("Hibernate")
            }
            QtControls.CheckBox {
                id: sleep
                text: i18n("Sleep")
            }
        }
    }
}
