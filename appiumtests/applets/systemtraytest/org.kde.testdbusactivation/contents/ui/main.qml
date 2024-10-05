/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: MIT
*/

import QtQuick
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid

PlasmoidItem {
    id: bluetoothApplet
    compactRepresentation: Rectangle { color: "red" }
    fullRepresentation: Rectangle { color: "green" }
    Plasmoid.status: PlasmaCore.Types.ActiveStatus
}
