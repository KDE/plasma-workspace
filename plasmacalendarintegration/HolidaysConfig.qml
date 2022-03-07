/*
    SPDX-FileCopyrightText: 2013 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.5
import QtQuick.Controls 1.4 as QQC1
import QtQuick.Controls 2.5 as QQC2
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.1

import org.kde.plasma.core 2.1 as PlasmaCore
import org.kde.kholidays 1.0 as KHolidays
import org.kde.holidayeventshelperplugin 1.0
import org.kde.kirigami 2.15 as Kirigami

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

    Kirigami.SearchField {
        id: filter
        Layout.fillWidth: true
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
