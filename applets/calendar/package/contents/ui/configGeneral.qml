/*
 * Copyright 2015 Martin Klapetek <mklapetek@kde.org>
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
    id: generalPage
    
    width: childrenRect.width
    height: childrenRect.height

    property alias cfg_showWeekNumbers: showWeekNumbers.checked
    property string cfg_compactDisplay

    QtLayouts.ColumnLayout {
        QtControls.CheckBox {
            id: showWeekNumbers
            text: i18n("Show week numbers in Calendar")
        }

        QtLayouts.RowLayout {
            QtControls.Label {
                text: i18nc("What information is shown in the calendar icon", "Icon:")
            }

            QtControls.ComboBox {
                id: compactDisplayCombo
                QtLayouts.Layout.minimumWidth: units.gridUnit * 7 // horrible default sizing in ComboBox
                model: [{
                    text: i18nc("Show the number of the day (eg. 31) in the icon", "Day in month"),
                    value: "d"
                }, {
                    text: i18nc("Show the week number (eg. 50) in the icon", "Week number"),
                    value: "w"
                }]
                onActivated: {
                    cfg_compactDisplay = compactDisplayCombo.model[index].value
                }

                Component.onCompleted: {
                    for (var i = 0, length = model.length; i < length; ++i) {
                        if (model[i].value === cfg_compactDisplay) {
                            currentIndex = i
                            return
                        }
                    }
                }
            }
        }
    }
}
