/*
 * Copyright 2018 Kai Uwe Broulik <kde@privat.broulik.de>
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

import QtQuick 2.8
import QtQuick.Layouts 1.1

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as Components
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.notificationmanager 1.0 as NotificationManager

Item {
    id: fullRoot

    // FIXME use PlasmaComponents 3 everywhere?
    PlasmaExtras.ScrollArea {
        anchors.fill: parent

        ListView {
            id: list
            // FIXME use history model or the finalized API with NotificationModel instance
            // that you can tel what you want (filtered, sorted by, etc), cf. TasksModel
            model: NotificationManager.NotificationModel

            delegate: NotificationDelegate {
                width: list.width

                summary: model.summary
                body: model.body
                icon: model.iconName || model.image
            }
        }
    }
}
