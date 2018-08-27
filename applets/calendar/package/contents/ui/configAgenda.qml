/*
 * Copyright 2013 Sebastian Kügler <sebas@kde.org>
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
import QtQuick.Layouts 1.0 as QtLayouts

Item {
    id: agendaPage
    
    width: childrenRect.width
    height: childrenRect.height
    implicitWidth: mainColumn.implicitWidth
    implicitHeight: pageColumn.implicitHeight

    property int cfg_startOfWorkingDay: startsAt.checked ? 0 : 9
    property int cfg_endOfWorkingDay: endsAt.checked ? 23 : 17

    QtLayouts.ColumnLayout {
        QtControls.GroupBox {
            title: i18n("Working Day")
            flat: true

            QtLayouts.ColumnLayout {
                QtControls.CheckBox {
                    id: startsAt
                    text: i18n("Starts at 9")
                }

                QtControls.CheckBox {
                    id: endsAt
                    text: i18n("Ends at 5")
                }
            }
        }
    }
}
