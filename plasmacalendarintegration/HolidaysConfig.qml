/*
    SPDX-FileCopyrightText: 2013 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2024 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

pragma ComponentBehavior: Bound

import QtQuick
import org.kde.kholidays as KHolidays
import org.kde.holidayeventshelperplugin
import org.kde.kitemmodels as KItemModels
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KirigamiDelegates
import org.kde.kcmutils as KCMUtils

KCMUtils.ScrollViewKCM {
    id: root

    signal configurationChanged()

    function saveConfig() {
        configHelper.saveConfig();
    }

    QmlConfigHelper {
        id: configHelper
    }

    header: Kirigami.SearchField {
        id: filter
    }

    view: ListView {
        id: holidaysView

        clip: true

        model: KItemModels.KSortFilterProxyModel {
            sourceModel: KHolidays.HolidayRegionsModel {}
            filterCaseSensitivity: Qt.CaseInsensitive
            filterString: filter.text
            filterRoleName: "name"
        }

        delegate: KirigamiDelegates.CheckSubtitleDelegate {
            required property string region
            required property string name
            required property string description

            text: name
            subtitle: description

            checked: configHelper.selectedRegions.includes(region)
            width: ListView.view.width
            icon.width: 0
            onClicked: {
                //needed for model's setData to be called
                if (checked) {
                    configHelper.addRegion(region);
                } else {
                    configHelper.removeRegion(region);
                }
                root.configurationChanged();
            }
        }
    }
}
