/*
 *   Copyright 2019 Marco Martin <mart@kde.org>
 *   Copyright 2019 David Edmundson <davidedmundson@kde.org>
 *   Copyright 2019 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.9
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.2
import QtQuick.Window 2.12
import QtGraphicalEffects 1.0
import QtQml 2.15

import org.kde.kirigami 2.8 as Kirigami

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.plasmoid 2.0

import org.kde.ksysguard.sensors 1.0 as Sensors
import org.kde.ksysguard.faces 1.0 as Faces

import org.kde.quickcharts 1.0 as Charts

Control {
    id: chartFace

    Layout.minimumWidth: (contentItem ? contentItem.Layout.minimumWidth : 0) + leftPadding + rightPadding
    Layout.minimumHeight: (contentItem ? contentItem.Layout.minimumHeight : 0) + leftPadding + rightPadding
    Layout.preferredWidth: (contentItem
            ? (contentItem.Layout.preferredWidth > 0 ? contentItem.Layout.preferredWidth : contentItem.implicitWidth)
            : 0) + leftPadding + rightPadding
    Layout.preferredHeight: (contentItem
            ? (contentItem.Layout.preferredHeight > 0 ? contentItem.Layout.preferredHeight: contentItem.implicitHeight)
            : 0) + leftPadding + rightPadding
    Layout.maximumWidth: (contentItem ? contentItem.Layout.maximumWidth : 0) + leftPadding + rightPadding
    Layout.maximumHeight: (contentItem ? contentItem.Layout.maximumHeight : 0) + leftPadding + rightPadding

    Kirigami.Theme.inherit: false
    Kirigami.Theme.textColor: PlasmaCore.ColorScope.textColor
    Kirigami.Theme.backgroundColor: PlasmaCore.ColorScope.backgroundColor
    Kirigami.Theme.disabledTextColor: PlasmaCore.ColorScope.disabledTextColor


    contentItem: plasmoid.nativeInterface.faceController.fullRepresentation

    Binding {
        target: plasmoid.nativeInterface.faceController.fullRepresentation
        property: "formFactor"
        value: {
            switch (plasmoid.formFactor) {
            case PlasmaCore.Types.Horizontal:
                return Faces.SensorFace.Horizontal;
            case PlasmaCore.Types.Vertical:
                return Faces.SensorFace.Verical;
            default:
                return Faces.SensorFace.Planar;
            }
        }
        restoreMode: Binding.RestoreBinding
    }
}

