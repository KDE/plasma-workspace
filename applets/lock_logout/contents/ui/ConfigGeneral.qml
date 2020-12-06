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
import org.kde.plasma.private.sessions 2.0

Kirigami.FormLayout {
    id: iconsPage
    anchors.left: parent.left
    anchors.right: parent.right

    readonly property int checkedOptions: logout.checked + shutdown.checked + reboot.checked + lock.checked + switchUser.checked + hibernate.checked + sleep.checked

    property alias cfg_show_requestLogout: logout.checked
    property alias cfg_show_requestShutDown: shutdown.checked
    property alias cfg_show_requestReboot: reboot.checked

    property alias cfg_show_lockScreen: lock.checked
    property alias cfg_show_switchUser: switchUser.checked
    property alias cfg_show_suspendToDisk: hibernate.checked
    property alias cfg_show_suspendToRam: sleep.checked

    SessionManagement {
        id: session
    }

    QtControls.CheckBox {
        id: logout
        Kirigami.FormData.label: i18nc("Heading for a list of actions (leave, lock, switch user, hibernate, suspend)", "Show actions:")
        text: i18n("Logout")
        // ensure user cannot have all options unchecked
        enabled: session.canLogout && (checkedOptions > 1 || !checked)
    }
    QtControls.CheckBox {
        id: shutdown
        text: i18n("Shutdown")
        enabled: session.canShutdown && (checkedOptions > 1 || !checked)
    }
    QtControls.CheckBox {
        id: reboot
        text: i18n("Reboot")
        enabled: session.canReboot && (checkedOptions > 1 || !checked)
    }
    QtControls.CheckBox {
        id: lock
        text: i18n("Lock")
        enabled: session.canLock && (checkedOptions > 1 || !checked)
    }
    QtControls.CheckBox {
        id: switchUser
        text: i18n("Switch User")
        enabled: checkedOptions > 1 || !checked
    }
    QtControls.CheckBox {
        id: hibernate
        text: i18n("Hibernate")
        enabled: session.canHibernate && (checkedOptions > 1 || !checked)
    }
    QtControls.CheckBox {
        id: sleep
        text: i18nc("Suspend to RAM", "Sleep")
        enabled: session.canSuspend && (checkedOptions > 1 || !checked)
    }
}
