/*
 * Copyright 2014 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

import QtQuick 2.0
import QtQuick.Controls 1.0 as QtControls
import QtQuick.Layouts 1.1 as QtLayouts
import QtQuick.Window 2.2
import org.kde.plasma.core 2.0 as PlasmaCore

import org.kde.plasma.private.notifications 1.0

Item {
    id: appearancePage
    width: childrenRect.width
    height: childrenRect.height

    signal configurationChanged

    property alias cfg_showNotifications: showNotificationsCheckBox.checked
    property alias cfg_showJobs: showJobsCheckBox.checked
    property alias cfg_showHistory: showHistoryCheckBox.checked

    QtLayouts.ColumnLayout {
        anchors.left: parent.left
        QtControls.CheckBox {
            id: showNotificationsCheckBox
            text: i18n("Show application and system notifications")
        }

        QtControls.CheckBox {
            id: showHistoryCheckBox
            text: i18n("Show a history of notifications")
            enabled: showNotificationsCheckBox.checked
        }

        QtControls.CheckBox {
            id: showJobsCheckBox
            text: i18n("Track file transfers and other jobs")
        }
        
        QtControls.CheckBox {
            id: useCustomPopupPositionCheckBox
            text: i18n("Use custom position for the notification popup")
            checked: plasmoid.nativeInterface.configScreenPosition() != NotificationsHelper.Default
        }

        ScreenPositionSelector {
            id: screenPositionSelector
            enabled: useCustomPopupPositionCheckBox.checked
            selectedPosition: plasmoid.nativeInterface.screenPosition
            disabledPositions: [NotificationsHelper.Left, NotificationsHelper.Center, NotificationsHelper.Right]
        }
    }

    Component.onCompleted: {
        plasmoid.nativeInterface.screenPosition = Qt.binding(function() {configurationChanged(); return screenPositionSelector.selectedPosition; });
    }
}
