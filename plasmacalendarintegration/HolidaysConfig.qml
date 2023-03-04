/*
    SPDX-FileCopyrightText: 2013 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts 1.3
import Qt.labs.qmlmodels as QMLModels

import org.kde.plasma.core 2.1 as PlasmaCore
import org.kde.kholidays 1.0 as KHolidays
import org.kde.holidayeventshelperplugin 1.0
import org.kde.kirigami 2.15 as Kirigami

ColumnLayout {
    id: holidaysConfig

    spacing: 0

    signal configurationChanged

    function saveConfig()
    {
        configHelper.saveConfig();
    }

    QmlConfigHelper {
        id: configHelper
    }

    QMLModels.DelegateChooser {
        id: chooser

        QMLModels.DelegateChoice {
            column: 0
            delegate: QQC2.CheckBox {
                activeFocusOnTab: false // only let the TableView as a whole get focus
                checked: model ? configHelper.selectedRegions.indexOf(model.region) !== -1 : false
                text: model.region

                onToggled: {
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

        QMLModels.DelegateChoice {
            column: 1
            delegate: QQC2.Label {
                text: model.display
                wrapMode: Text.Wrap
            }
        }

        QMLModels.DelegateChoice {
            column: 2
            delegate: QQC2.Label {
                text: model.display
                wrapMode: Text.Wrap
            }
        }
    }

    Kirigami.SearchField {
        id: filter
        Layout.fillWidth: true
    }

    QQC2.HorizontalHeaderView {
        id: horizontalHeader
        Layout.fillWidth: true
        Layout.topMargin: Kirigami.Units.largeSpacing
        syncView: holidaysView
    }

    TableView {
        id: holidaysView

        signal toggleCurrent

        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.topMargin: rowSpacing

        rowSpacing: Kirigami.Units.smallSpacing
        columnSpacing: Kirigami.Units.smallSpacing

        Keys.onSpacePressed: toggleCurrent()

        clip: true
        columnWidthProvider: function(column) {
            switch (column) {
            case 1:
                return 0.4 * holidaysConfig.width;
            case 2:
                return 0.4 * holidaysConfig.width - holidaysView.columnSpacing * 2;
            case 0:
            default:
                return 0.2 * holidaysConfig.width;
            }
        }
        delegate: chooser
        model: PlasmaCore.SortFilterModel {
            sourceModel: KHolidays.HolidayRegionsModel {
                id: holidaysModel
            }
            // SortFilterModel doesn't have a case-sensitivity option...
            // but filterRegExp always causes case-insensitive sorting
            filterRegExp: filter.text
            filterRole: "name"
        }
        reuseItems: true

        QMLModels.TableModelColumn {
            display: "region"
        }
        QMLModels.TableModelColumn {
            display: "name"
        }
        QMLModels.TableModelColumn {
            display: "description"
        }
    }
}
