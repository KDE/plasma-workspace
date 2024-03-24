/*
    SPDX-FileCopyrightText: 2013 Heena Mahour <heena393@gmail.com>
    SPDX-FileCopyrightText: 2013 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2024 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQml.Models
import org.kde.plasma.extras as PlasmaExtras

PlasmaExtras.Menu {
    id: root

    property int year

    // Needs to be a property since Menu doesn't accept other items than MenuItem
    readonly property Instantiator __items: Instantiator {
        model: 12

        PlasmaExtras.MenuItem {
            required property int index

            text: root.capitalizeFirstLetter(Qt.locale(Qt.locale().uiLanguages[0]).standaloneMonthName(index))

            onClicked: calendarBackend.displayedDate = new Date(root.year, index, 1)
        }

        onObjectAdded: (index, object) => root.addMenuItem(object)
    }

    // Because some locales don't have it in standaloneMonthNames,
    // but we want our GUI to be pretty and want capitalization always
    function capitalizeFirstLetter(monthName: string): string {
        return monthName.charAt(0).toUpperCase() + monthName.slice(1);
    }
}
