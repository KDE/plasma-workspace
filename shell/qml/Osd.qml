/*
    SPDX-FileCopyrightText: 2014 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2026 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import org.kde.kirigami as Kirigami

OsdWindow {

    LayoutMirroring.enabled: Application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    property alias timeout: osd.timeout
    property alias osdValue: osd.osdValue
    property alias osdMaxValue: osd.osdMaxValue
    property alias osdAdditionalText: osd.osdAdditionalText
    property alias icon: osd.icon
    property alias showingProgress: osd.showingProgress

    width: Math.max(Math.min(Screen.desktopAvailableWidth / 2, mainItem.implicitWidth), Kirigami.Units.gridUnit * 15) +  leftPadding + rightPadding
    height: mainItem.implicitHeight + topPadding + bottomPadding

    mainItem: OsdItem {
        id: osd
    }
}
