/*
 * Copyright 2017, 2019 Kai Uwe Broulik <kde@privat.broulik.de>
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

import QtQuick 2.2

import org.kde.plasma.private.volume 0.1

QtObject {
    id: pulseAudio

    readonly property string notificationStreamId: "sink-input-by-media-role:event"

    property QtObject notificationStream

    property QtObject instantiator: Instantiator {
        model: StreamRestoreModel {}

        delegate: QtObject {
            readonly property string name: Name
            readonly property bool muted: Muted

            function mute() {
                Muted = true
            }
            function unmute() {
                Muted = false
            }
        }

        onObjectAdded: {
            if (object.name === notificationStreamId) {
                notificationStream = object;
            }
        }
    }
}
