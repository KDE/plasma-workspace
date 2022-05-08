/*
    SPDX-FileCopyrightText: 2013 Heena Mahour <heena393@gmail.com>
    SPDX-FileCopyrightText: 2013 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.2
import org.kde.plasma.components 2.0 as PlasmaComponents

PlasmaComponents.Menu {
    id: testMenu
    property int year

    // Needs to be a property since Menu doesn't accept other items than MenuItem
    property Instantiator items: Instantiator {
        model: 12
        PlasmaComponents.MenuItem {
            text: capitalizeFirstLetter(Qt.locale(Qt.locale().uiLanguages[0]).standaloneMonthName(index))
            onClicked: calendarBackend.displayedDate = new Date(year, index, 1)
        }
        onObjectAdded: testMenu.addMenuItem(object)
    }

    // Because some locales don't have it in standaloneMonthNames,
    // but we want our GUI to be pretty and want capitalization always
    function capitalizeFirstLetter(monthName) {
        return monthName.charAt(0).toUpperCase() + monthName.slice(1);
    }
}
