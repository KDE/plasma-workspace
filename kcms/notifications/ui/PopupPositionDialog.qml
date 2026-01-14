/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2025 Akseli Lahtinen <akselmo@akselmo.dev>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QtControls
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: positionPopup
    title: i18n("Popup Position")
    showCloseButton: false
    padding: Kirigami.Units.largeSpacing

    property var newPosition

    contentItem: ScreenPositionSelector {
        id: positionSelector
        selectedPosition: kcm.notificationSettings.popupPosition
        onSelectedPositionChanged: positionPopup.newPosition = selectedPosition
    }

    footer: QtControls.DialogButtonBox {
        standardButtons: QtControls.DialogButtonBox.Ok | QtControls.DialogButtonBox.Cancel
        onAccepted: {
            kcm.notificationSettings.popupPosition = positionPopup.newPosition
            positionPopup.close()
        }
        onRejected: {
            positionSelector.selectedPosition = kcm.notificationSettings.popupPosition
            positionPopup.close()
        }
    }
}
