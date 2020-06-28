/*
 * Copyright 2013 Kai Uwe Broulik <kde@privat.broulik.de>
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

import QtQuick 2.5
import QtQuick.Controls 1.4 as QQC1
import QtQuick.Controls 2.5 as QQC2
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.1

import org.kde.plasma.core 2.1 as PlasmaCore
import org.kde.kholidays 1.0 as KHolidays
import org.kde.holidayeventshelperplugin 1.0

ColumnLayout {
    id: holidaysConfig
    anchors.left: parent.left
    anchors.right: parent.right

    signal configurationChanged

    function saveConfig()
    {
        configHelper.saveConfig();
    }

    // This is just for getting the column width
    QQC2.CheckBox {
        id: checkbox
        visible: false
    }

    QmlConfigHelper {
        id: configHelper
    }

    QQC2.TextField {
        id: filter
        Layout.fillWidth: true
        placeholderText: i18nd("kholidays_calendar_plugin", "Search...")
    }

    // Still QQC1 bevcause there's no QQC2 TableView
    QQC1.TableView {
        id: holidaysView

        signal toggleCurrent

        Layout.fillWidth: true
        Layout.fillHeight: true

        Keys.onSpacePressed: toggleCurrent()

        model: PlasmaCore.SortFilterModel {
            sourceModel: KHolidays.HolidayRegionsModel {
                id: holidaysModel
            }
            // SortFilterModel doesn't have a case-sensitivity option...
            // but filterRegExp always causes case-insensitive sorting
            filterRegExp: filter.text
            filterRole: "name"
        }

        QQC1.TableViewColumn {
            width: checkbox.width
            delegate: QQC2.CheckBox {
                id: checkBox
                anchors.centerIn: parent
                checked: model ? configHelper.selectedRegions.indexOf(model.region) !== -1 : false
                activeFocusOnTab: false // only let the TableView as a whole get focus
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

            resizable: false
            movable: false
        }
        QQC1.TableViewColumn {
            role: "region"
            title: i18nd("kholidays_calendar_plugin", "Region")
        }
        QQC1.TableViewColumn {
            role: "name"
            title: i18nd("kholidays_calendar_plugin", "Name")
        }
        QQC1.TableViewColumn {
            role: "description"
            title: i18nd("kholidays_calendar_plugin", "Description")
        }
    }
}
