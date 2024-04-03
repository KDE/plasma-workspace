/*
    SPDX-FileCopyrightText: 2013 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2024 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kholidays as KHolidays
import org.kde.holidayeventshelperplugin
import org.kde.kitemmodels as KItemModels
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD
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

        KeyNavigation.down: holidaysView
    }

    view: ListView {
        id: holidaysView

        activeFocusOnTab: true
        clip: true
        reuseItems: true

        model: KItemModels.KSortFilterProxyModel {
            sourceModel: KHolidays.HolidayRegionsModel {}
            filterCaseSensitivity: Qt.CaseInsensitive
            filterString: filter.text
            filterRoleName: "name"
        }

        delegate: QQC2.CheckDelegate {
            id: delegate

            required property string region
            required property string name
            required property string description

            property string subtitle

            text: name
            subtitle: description

            checked: configHelper.selectedRegions.includes(region)
            width: ListView.view.width
            icon.width: 0

            contentItem: KD.IconTitleSubtitle {
                icon: icon.fromControlsIcon(delegate.icon)
                title: delegate.text
                subtitle: delegate.subtitle
                selected: delegate.highlighted || delegate.down
                font: delegate.font

                // These two lines are the only reason to roll out such custom delegate
                reserveSpaceForSubtitle: true
                wrapMode: Text.Wrap
            }

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
