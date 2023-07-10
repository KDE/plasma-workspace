/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2019 Arjen Hiemstra <ahiemstra@heimr.nl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.9
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.2
import QtQml 2.15

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore

import org.kde.ksysguard.faces 1.0 as Faces

import org.kde.kirigami 2.8 as Kirigami

Control {
    id: chartFace
    Layout.fillWidth: contentItem ? contentItem.Layout.fillWidth : false
    Layout.fillHeight: contentItem ? contentItem.Layout.fillHeight : false

    Layout.minimumWidth: (contentItem ? contentItem.Layout.minimumWidth : 0) + leftPadding + rightPadding
    Layout.minimumHeight: (contentItem ? contentItem.Layout.minimumHeight : 0) + leftPadding + rightPadding

    Layout.preferredWidth: (contentItem ? contentItem.Layout.preferredWidth : 0) + leftPadding + rightPadding
    Layout.preferredHeight: (contentItem ? contentItem.Layout.preferredHeight : 0) + leftPadding + rightPadding

    Layout.maximumWidth: (contentItem ? contentItem.Layout.maximumWidth : 0) + leftPadding + rightPadding
    Layout.maximumHeight: (contentItem ? contentItem.Layout.maximumHeight : 0) + leftPadding + rightPadding

    leftPadding: 0
    topPadding: 0
    rightPadding: 0
    bottomPadding: 0

    anchors.fill: parent
    contentItem: Plasmoid.faceController.compactRepresentation

    Binding {
        target: Plasmoid.faceController.compactRepresentation
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

    MouseArea {
        parent: chartFace
        anchors.fill: parent
        onClicked: root.expanded = !root.expanded
    }
}
