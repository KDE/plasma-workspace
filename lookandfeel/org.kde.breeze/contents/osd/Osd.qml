/*
    SPDX-FileCopyrightText: 2014 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick 2.0
import QtQuick.Window 2.2
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtra

PlasmaCore.Dialog {
    id: root
    location: PlasmaCore.Types.Floating
    type: PlasmaCore.Dialog.OnScreenDisplay
    outputOnly: true

    property alias timeout: osd.timeout
    property alias osdValue: osd.osdValue
    property alias osdMaxValue: osd.osdMaxValue
    property alias icon: osd.icon
    property alias showingProgress: osd.showingProgress

    mainItem: OsdItem {
        id: osd
    }
}
