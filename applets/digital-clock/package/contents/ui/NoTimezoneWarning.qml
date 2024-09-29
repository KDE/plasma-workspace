/*
 SPDX-FileCopyrightText: 2024 Niccolò Venerandi <niccolo@venerandi.com>

 SPDX-License-Identifier: GPL-2.0-or-later
 */
import QtQuick
import QtQuick.Layouts

import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.plasmoid 2.0
import org.kde.kirigami 2.20 as Kirigami

import org.kde.kcmutils as KCM

MouseArea {
    id: root

    Layout.minimumWidth: mainLayout.implicitWidth
    Layout.minimumHeight: mainLayout.implicitHeight

    onClicked: KCM.KCMLauncher.openSystemSettings("kcm_clock")


    RowLayout {
        id: mainLayout
        anchors.fill: parent
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Icon {
            source: "appointment-missed-symbolic"
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        PlasmaComponents.Label {
            text: i18nc("@label Shown in place of digital clock when no timezone is set", "Time zone is not set; click here to open Date & Time settings and set one")
            visible: Plasmoid.formFactor == PlasmaCore.Types.Horizontal
        }
    }
}
