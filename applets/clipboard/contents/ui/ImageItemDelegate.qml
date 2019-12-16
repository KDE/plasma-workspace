/********************************************************************
This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>
Copyright     2014 Sebastian Kügler <sebas@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

import QtQuick 2.0

import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons

KQuickControlsAddons.QPixmapItem {
    id: previewPixmap
    width: Math.min(nativeWidth, width)
    height: Math.min(nativeHeight, Math.round(width * (nativeHeight/nativeWidth) + units.smallSpacing * 2))
    pixmap: DecorationRole
    fillMode: KQuickControlsAddons.QPixmapItem.PreserveAspectFit
}
