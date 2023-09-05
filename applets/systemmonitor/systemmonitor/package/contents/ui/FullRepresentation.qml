/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2019 Arjen Hiemstra <ahiemstra@heimr.nl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.9
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.2
import QtQuick.Window 2.12
import Qt5Compat.GraphicalEffects
import QtQml 2.15

import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid 2.0

import org.kde.ksysguard.faces 1.0 as Faces


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
                return Faces.SensorFace.Verical;
            default:
                return Faces.SensorFace.Planar;
            }
        }
        restoreMode: Binding.RestoreBinding
    }
}

