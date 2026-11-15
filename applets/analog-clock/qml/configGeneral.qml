/*
    SPDX-FileCopyrightText: 2013 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    property alias cfg_showSecondHand: showSecondHandCheckBox.checked
    property alias cfg_showTimezoneString: showTimezoneCheckBox.checked

    Kirigami.Form {
        Kirigami.FormGroup {
            Kirigami.FormEntry {
                title: i18nc("@title:group", "General:")
                contentItem: CheckBox {
                    id: showSecondHandCheckBox
                    text: i18nc("@option:check", "Show seconds hand")
                }
            }
            Kirigami.FormEntry {
                contentItem: CheckBox {
                    id: showTimezoneCheckBox
                    text: i18nc("@option:check", "Show time zone")
                }
            }
        }
    }
}
