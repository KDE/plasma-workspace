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

    header: ColumnLayout {
        Kirigami.SearchField {
            id: filter
            Layout.fillWidth: true
        }
    }


    view: TableView {
        id: holidaysView

        signal toggleCurrent

        Keys.onSpacePressed: toggleCurrent()

        QQC2.HorizontalHeaderView {
            id: horizontalHeader
            syncView: holidaysView

            parent: holidaysView

            model: [
                "",
                i18nc("@label", "Name"),
                i18nc("@label", "Description"),
            ]

            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }
        }

        topMargin: horizontalHeader.height

        clip: true

        model: KItemModels.KSortFilterProxyModel {
            sourceModel: KHolidays.HolidayRegionsModel {
                id: holidaysModel
            }
            filterCaseSensitivity: Qt.CaseInsensitive
            filterString: filter.text
            filterRoleName: "name"
        }

        delegate: DelegateChooser {
            role: "columnName"

            DelegateChoice {
                roleValue: "region"

                QQC2.CheckBox {
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
            }

            DelegateChoice {
                QQC2.ItemDelegate {
                    text: model.display
                }
            }
        }

/*
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
        */
    }
}
