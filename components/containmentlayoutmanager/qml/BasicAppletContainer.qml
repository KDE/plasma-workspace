/*
 *  Copyright 2019 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.12
import QtQuick.Layouts 1.2

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.plasma.private.containmentlayoutmanager 1.0 as ContainmentLayoutManager

ContainmentLayoutManager.AppletContainer {
    id: appletContainer
    editModeCondition: plasmoid.immutable
        ? ContainmentLayoutManager.ItemContainer.Manual
        : ContainmentLayoutManager.ItemContainer.AfterPressAndHold

    Layout.minimumWidth: {
        if (!applet) {
            return leftPadding + rightPadding;
        }

        if (applet.preferredRepresentation != applet.fullRepresentation
            && applet.compactRepresentationItem
        ) {
            return applet.compactRepresentationItem.Layout.minimumWidth + leftPadding + rightPadding;
        } else {
            return applet.Layout.minimumWidth + leftPadding + rightPadding;
        }
    }
    Layout.minimumHeight: {
        if (!applet) {
            return topPadding + bottomPadding;
        }

        if (applet.preferredRepresentation != applet.fullRepresentation
            && applet.compactRepresentationItem
        ) {
            return applet.compactRepresentationItem.Layout.minimumHeight + topPadding + bottomPadding;
        } else {
            return applet.Layout.minimumHeight + topPadding + bottomPadding;
        }
    }

    Layout.preferredWidth: Math.max(applet.Layout.minimumWidth, applet.Layout.preferredWidth)
    Layout.preferredHeight: Math.max(applet.Layout.minimumHeight, applet.Layout.preferredHeight)

    Layout.maximumWidth: applet.Layout.maximumWidth
    Layout.maximumHeight: applet.Layout.maximumHeight

    leftPadding: background.margins.left
    topPadding: background.margins.top
    rightPadding: background.margins.right
    bottomPadding: background.margins.bottom

    initialSize.width: applet.switchWidth + leftPadding + rightPadding
    initialSize.height: applet.switchHeight + topPadding + bottomPadding

    background: PlasmaCore.FrameSvgItem {
        imagePath: contentItem && contentItem.backgroundHints == PlasmaCore.Types.StandardBackground ? "widgets/background" : ""
    }

    busyIndicatorComponent: PlasmaComponents.BusyIndicator {
        anchors.centerIn: parent
        visible: applet.busy
        running: visible
    }
} 
