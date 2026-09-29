/*
    SPDX-FileCopyrightText: 2026 Ameen Al-Asady <ameenaladdin@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

pragma Singleton
import QtQuick

QtObject {
    property string lastAutoOpenedUdi: ""
    property double lastAutoOpenTime: 0

    // Must outlast the popup opening, as both instances receive the event back-to-back.
    readonly property int claimWindowMs: 1000

    // Succeeds once per device event, so only the first instance here auto-opens.
    function tryAutoOpen(udi: string): bool {
        if (udi === "") {
            return false;
        }
        const now = Date.now();
        if (udi === lastAutoOpenedUdi && now - lastAutoOpenTime < claimWindowMs) {
            return false;
        }
        lastAutoOpenedUdi = udi;
        lastAutoOpenTime = now;
        return true;
    }
}
