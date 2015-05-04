/*
 * Copyright 2013 Sebastian Kügler <sebas@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.calendar 2.0 as PlasmaCalendar
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    /******************************************************************
     *
     * TODO: Revert commit bfd62154d8e892d4fdc87d27d25d07cb7841c1e6
     *       to bring back the original agenda part
     *
     ******************************************************************/

    id: calendar

    Layout.minimumWidth: _minimumWidth
    Layout.minimumHeight: _minimumHeight

    // The "sensible" values
    property int _minimumWidth: Math.round(_minimumHeight * 1.5)
    property int _minimumHeight: units.gridUnit * 14
    Layout.preferredWidth: _minimumWidth
    Layout.preferredHeight: Math.round(_minimumHeight * 1.5)

    property int avWidth: Math.round(parent.width - (1.5 * units.largeSpacing))
    property int avHeight: parent.height - (2 * units.largeSpacing)

    //anchors.margins: units.largeSpacing
    property int spacing: units.largeSpacing
    property alias borderWidth: monthView.borderWidth
    property alias monthView: monthView

    property bool debug: false

    property bool isExpanded: plasmoid.expanded

    onIsExpandedChanged: {
        if (!isExpanded) {
            // clear all the selections when the plasmoid is hiding
            monthView.date = null;
            monthView.resetToToday();
        }
    }

    Item {
        id: cal
        width: avWidth
        anchors.fill: parent
        anchors.margins: spacing

        PlasmaCalendar.MonthView {
            id: monthView
            borderOpacity: 0.25
            today: root.tzDate
            anchors.fill: parent
        }

    }

    // Allows the user to keep the calendar open for reference
    PlasmaComponents.ToolButton {
        anchors.right: parent.right
        width: Math.round(units.gridUnit * 1.25)
        height: width
        checkable: true
        iconSource: "window-pin"
        onCheckedChanged: plasmoid.hideOnWindowDeactivate = !checked
    }

}
