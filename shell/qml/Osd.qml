/*
    SPDX-FileCopyrightText: 2014 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2026 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import org.kde.kirigami as Kirigami

import org.kde.plasma.workspace.osd

OsdWindow {

    LayoutMirroring.enabled: Application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    property alias timeout: osd.timeout
    property alias osdValue: osd.osdValue
    property alias osdMaxValue: osd.osdMaxValue
    property alias osdAdditionalText: osd.osdAdditionalText
    property alias icon: osd.icon
    property alias showingProgress: osd.showingProgress

    // Osd::showOsd() applies these again before every show: the bindings below don't do it
    // if the compositor resized the window while the content size stayed the same
    readonly property int preferredWidth: Math.max(Math.min(Screen.desktopAvailableWidth / 2, mainItem.implicitWidth), Kirigami.Units.gridUnit * 15) +  leftPadding + rightPadding
    readonly property int preferredHeight: mainItem.implicitHeight + topPadding + bottomPadding

    width: preferredWidth
    height: preferredHeight

    mainItem: OsdItem {
        id: osd
    }
}
