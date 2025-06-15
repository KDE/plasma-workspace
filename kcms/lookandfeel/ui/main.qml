/*
    SPDX-FileCopyrightText: 2018 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2022 Dominic Hayes <ferenosdev@outlook.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

import QtQuick
import QtQuick.Layouts
import org.kde.newstuff as NewStuff
import org.kde.kcmutils as KCM

KCM.AbstractKCM {
    id: root

    actions: [
        NewStuff.Action {
            configFile: "lookandfeel.knsrc"
            text: i18n("Get New…")
            onEntryEvent: function (entry, event) {
                if (event == NewStuff.Entry.StatusChangedEvent) {
                    kcm.knsEntryChanged(entry);
                } else if (event == NewStuff.Entry.AdoptedEvent) {
                    kcm.reloadConfig();
                }
            }
        }
    ]

    SingleLookAndFeel {
        anchors.fill: parent
    }
}
