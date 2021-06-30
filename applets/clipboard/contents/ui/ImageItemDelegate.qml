/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0

import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons
import org.kde.plasma.core 2.0 as PlasmaCore

KQuickControlsAddons.QPixmapItem {
    id: previewPixmap
    width: Math.min(nativeWidth, width)
    height: Math.min(nativeHeight, Math.round(width * (nativeHeight/nativeWidth) + PlasmaCore.Units.smallSpacing * 2))
    pixmap: DecorationRole
    fillMode: KQuickControlsAddons.QPixmapItem.PreserveAspectFit
}
