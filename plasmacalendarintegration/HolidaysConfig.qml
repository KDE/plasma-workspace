/*
    SPDX-FileCopyrightText: 2013 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import Qt.labs.qmlmodels
import org.kde.kholidays as KHolidays
import org.kde.holidayeventshelperplugin
import org.kde.kitemmodels as KItemModels
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KirigamiDelegates
import org.kde.kcmutils as KCMUtils

KCMUtils.ScrollViewKCM {
    id: holidaysConfig

    signal configurationChanged

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

        signal toggleCurrent

        Keys.onSpacePressed: toggleCurrent()

        clip: true

        model: KItemModels.KSortFilterProxyModel {
            sourceModel: KHolidays.HolidayRegionsModel {
                id: holidaysModel
            }
            filterCaseSensitivity: Qt.CaseInsensitive
            filterString: filter.text
            filterRoleName: "name"
        }

        delegate: KirigamiDelegates.CheckSubtitleDelegate {
            text: model.name
            subtitle: model.description

            checked: model ? configHelper.selectedRegions.indexOf(model.region) !== -1 : false
            width: ListView.view.width
            onClicked: {
                //needed for model's setData to be called
                if (checked) {
                    configHelper.addRegion(model.region);
                } else {
                    configHelper.removeRegion(model.region);
                }
                holidaysConfig.configurationChanged();
            }
        }
    }
}
