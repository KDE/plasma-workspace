/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.0
import QtQuick.Controls 2.4 as QtControls
import QtQuick.Layouts
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.workspace.calendar 2.0 as PlasmaCalendar
import org.kde.kirigami 2.5 as Kirigami

ColumnLayout {
    id: calendarPage

    signal configurationChanged

    property alias cfg_showWeekNumbers: showWeekNumbers.checked
    property int cfg_firstDayOfWeek

    function saveConfig()
    {
        Plasmoid.configuration.enabledCalendarPlugins = eventPluginsManager.enabledPlugins;
    }

    PlasmaCalendar.EventPluginsManager {
        id: eventPluginsManager
        Component.onCompleted: {
            populateEnabledPluginsList(Plasmoid.configuration.enabledCalendarPlugins);
        }
    }

    Kirigami.FormLayout {
        Layout.fillWidth: true

        QtControls.CheckBox {
            id: showWeekNumbers
            Kirigami.FormData.label: i18n("General:")
            text: i18n("Show week numbers")
        }

        RowLayout {
            Layout.fillWidth: true
            Kirigami.FormData.label: i18n("First day of week:")

            QtControls.ComboBox {
                id: firstDayOfWeekCombo
                textRole: "text"
                model: [-1, 0, 1, 5, 6].map((day) => {
                    return {
                        day,
                        text: day === -1 ? i18n("Use Region Defaults") : Qt.locale().dayName(day)
                    };
                })
                onActivated: cfg_firstDayOfWeek = model[index].day
                currentIndex: model.findIndex((item) => {
                    return item.day === cfg_firstDayOfWeek;
                })
            }
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        ColumnLayout {
            id: calendarPluginsLayout

            Kirigami.FormData.label: i18n("Available Plugins:")

            Repeater {
                id: calendarPluginsRepeater

                model: eventPluginsManager.model

                delegate: QtControls.CheckBox {
                    text: model.display
                    checked: model.checked
                    onClicked: {
                        //needed for model's setData to be called
                        model.checked = checked;
                        calendarPage.configurationChanged();
                    }
                }

                onItemAdded: (index, item) => {
                    if (index === 0) {
                        // Set buddy once, for an item in the first row.
                        // No, it doesn't work as a binding on children list.
                        calendarPluginsLayout.Kirigami.FormData.buddyFor = item;
                    }
                }
            }
        }
    }
}

