/*
 * Copyright 2013  Bhushan Shah <bhush94@gmail.com>
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
    id: appearancePage
    width: childrenRect.width
    height: childrenRect.height
    implicitWidth: mainColumn.implicitWidth
    implicitHeight: pageColumn.implicitHeight

    property alias cfg_boldText: boldCheckBox.checked
    property string cfg_timeFormat: ""
    property alias cfg_italicText: italicCheckBox.checked

    property alias cfg_showTimezone: showTimezone.checked
    property alias cfg_showSeconds: showSeconds.checked

    QtLayouts.ColumnLayout {
        QtControls.GroupBox {
            title: i18n("Appearance")
            flat: true

            QtLayouts.ColumnLayout {
                QtControls.CheckBox {
                    id: boldCheckBox
                    text: i18n("Bold text")
                }

                QtControls.CheckBox {
                    id: italicCheckBox
                    text: i18n("Italic text")
                }
            }
        }

        QtControls.GroupBox {
            title: i18n("Information")
            flat: true

            QtLayouts.ColumnLayout {
                QtControls.CheckBox {
                    id: showSeconds
                    text: i18n("Show seconds")
                }

                QtControls.CheckBox {
                    id: showTimezone
                    text: i18n("Show time zone")
                }
            }
        }
    }
}