/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2015 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0
import QtQuick.Controls 2.5 as QtControls
import org.kde.kirigami 2.5 as Kirigami
import org.kde.plasma.private.sessions 2.0
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    readonly property int checkedOptions: logout.checked + logoutScreen.checked + shutdown.checked + reboot.checked + lock.checked + switchUser.checked + hibernate.checked + sleep.checked

    property alias cfg_show_requestLogoutScreen: logoutScreen.checked
    property alias cfg_show_requestLogout: logout.checked
    property alias cfg_show_requestShutDown: shutdown.checked
    property alias cfg_show_requestReboot: reboot.checked

    property alias cfg_show_lockScreen: lock.checked
    property alias cfg_show_switchUser: switchUser.checked
    property alias cfg_show_suspendToDisk: hibernate.checked
    property alias cfg_show_suspendToRam: sleep.checked

    Kirigami.FormLayout {
        SessionManagement {
            id: session
        }

        QtControls.CheckBox {
            id: logout
            Kirigami.FormData.label: i18nc("Heading for a list of actions (leave, lock, switch user, hibernate, suspend)", "Show actions:")
            text: i18n("Log Out")
            icon.name: "system-log-out"
            // ensure user cannot have all options unchecked
            enabled: session.canLogout && (checkedOptions > 1 || !checked)
        }
        QtControls.CheckBox {
            id: logoutScreen
            text: i18nc("@option:check", "Show logout screen")
            icon.name: "system-log-out"
            enabled: session.canLogout && (checkedOptions > 1 || !checked)
        }
        QtControls.CheckBox {
            id: shutdown
            text: i18n("Shut Down")
            icon.name: "system-shutdown"
            enabled: session.canShutdown && (checkedOptions > 1 || !checked)
        }
        QtControls.CheckBox {
            id: reboot
            text: i18n("Restart")
            icon.name: "system-reboot"
            enabled: session.canReboot && (checkedOptions > 1 || !checked)
        }
        QtControls.CheckBox {
            id: lock
            text: i18n("Lock")
            icon.name: "system-lock-screen"
            enabled: session.canLock && (checkedOptions > 1 || !checked)
        }
        QtControls.CheckBox {
            id: switchUser
            text: i18n("Switch User")
            icon.name: "system-switch-user"
            enabled: checkedOptions > 1 || !checked
        }
        QtControls.CheckBox {
            id: hibernate
            text: i18n("Hibernate")
            icon.name: "system-suspend-hibernate"
            enabled: session.canHibernate && (checkedOptions > 1 || !checked)
        }
        QtControls.CheckBox {
            id: sleep
            text: i18nc("Suspend to RAM", "Sleep")
            icon.name: "system-suspend"
            enabled: session.canSuspend && (checkedOptions > 1 || !checked)
        }
    }
}
