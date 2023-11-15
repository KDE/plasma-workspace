/*
    SPDX-FileCopyrightText: 2013 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0
import QtQuick.Controls 2.0
import org.kde.kirigami 2.5 as Kirigami
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    property alias cfg_showSecondHand: showSecondHandCheckBox.checked
    property alias cfg_showTimezoneString: showTimezoneCheckBox.checked

    Kirigami.FormLayout {
        CheckBox {
            id: showSecondHandCheckBox
            text: i18n("Show seconds hand")
            Kirigami.FormData.label: i18n("General:")
        }
        CheckBox {
            id: showTimezoneCheckBox
            text: i18n("Show time zone")
        }
    }
}
