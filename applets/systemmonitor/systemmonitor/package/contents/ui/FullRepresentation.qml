/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2019 Arjen Hiemstra <ahiemstra@heimr.nl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Window
import Qt5Compat.GraphicalEffects
import QtQml

import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid

import org.kde.ksysguard.faces as Faces


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

    contentItem: Plasmoid.faceController.fullRepresentation

    // This empty mousearea serves for the sole purpose of refusing touch events
    // which otherwise are eaten by Control stealing the event from any of its parents
    // TODO KF6: Check if this is still needed as Qt6 doesn't accept touch by default on Control
    MouseArea {
        parent: chartFace
        anchors.fill:parent
    }

    Binding {
        target: Plasmoid.faceController.fullRepresentation
        property: "formFactor"
        value: {
            switch (Plasmoid.formFactor) {
            case PlasmaCore.Types.Horizontal:
                return Faces.SensorFace.Horizontal;
            case PlasmaCore.Types.Vertical:
                return Faces.SensorFace.Vertical;
            default:
                return Faces.SensorFace.Planar;
            }
        }
        restoreMode: Binding.RestoreBinding
    }
}

