/*
 *  Copyright 2016 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
 */

import QtQuick 2.0
import QtQuick.Controls 1.1 as Controls
import QtQuick.Layouts 1.1 as Layouts

Layouts.ColumnLayout {
    property alias cfg_pauseWhenScreenLocked: pauseWhenScreenLockedCheckBox.checked

    Controls.CheckBox {
        id: pauseWhenScreenLockedCheckBox
        Layouts.Layout.fillWidth: true
        text: i18n("Pause playback when screen is locked")
    }

    Item { // compress layout
        Layouts.Layout.fillHeight: true
    }
}
