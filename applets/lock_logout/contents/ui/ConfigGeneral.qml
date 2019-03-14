/*
 *  Copyright 2013 Sebastian Kügler <sebas@kde.org>
 *  Copyright 2015 Kai Uwe Broulik <kde@privat.broulik.de>
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
import QtQuick.Controls 2.5 as QtControls
import org.kde.kirigami 2.5 as Kirigami
import org.kde.plasma.core 2.0 as PlasmaCore

Kirigami.FormLayout {
    id: iconsPage
    anchors.left: parent.left
    anchors.right: parent.right

    readonly property int checkedOptions: leave.checked + lock.checked + switchUser.checked + hibernate.checked + sleep.checked

    property alias cfg_show_requestShutDown: leave.checked
    property alias cfg_show_lockScreen: lock.checked
    property alias cfg_show_switchUser: switchUser.checked
    property alias cfg_show_suspendToDisk: hibernate.checked
    property alias cfg_show_suspendToRam: sleep.checked

    readonly property bool canLockScreen: dataEngine.data["Sleep States"].LockScreen
    readonly property bool canSuspend: dataEngine.data["Sleep States"].Suspend
    readonly property bool canHibernate: dataEngine.data["Sleep States"].Hibernate

    PlasmaCore.DataSource {
        id: dataEngine
        engine: "powermanagement"
        connectedSources: ["Sleep States"]
    }

    QtControls.CheckBox {
        id: leave
        Kirigami.FormData.label: i18nc("Heading for a list of actions (leave, lock, switch user, hibernate, suspend)", "Show actions:")
        text: i18n("Leave")
        // ensure user cannot have all options unchecked
        enabled: checkedOptions > 1 || !checked
    }
    QtControls.CheckBox {
        id: lock
        text: i18n("Lock")
        enabled: iconsPage.canLockScreen && (checkedOptions > 1 || !checked)
    }
    QtControls.CheckBox {
        id: switchUser
        text: i18n("Switch User")
        enabled: checkedOptions > 1 || !checked
    }
    QtControls.CheckBox {
        id: hibernate
        text: i18n("Hibernate")
        enabled: iconsPage.canHibernate && (checkedOptions > 1 || !checked)
    }
    QtControls.CheckBox {
        id: sleep
        text: i18nc("Suspend to RAM", "Sleep")
        enabled: iconsPage.canSuspend && (checkedOptions > 1 || !checked)
    }
}
