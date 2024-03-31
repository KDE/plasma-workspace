/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.plasma.plasmoid
import org.kde.plasma.workspace.calendar as PlasmaCalendar
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCMUtils

KCMUtils.SimpleKCM {
    id: calendarPage

    signal configurationChanged()

    property alias cfg_showWeekNumbers: showWeekNumbers.checked
    property int cfg_firstDayOfWeek

    function saveConfig() {
        Plasmoid.configuration.enabledCalendarPlugins = eventPluginsManager.enabledPlugins;
    }

    Kirigami.FormLayout {
        PlasmaCalendar.EventPluginsManager {
            id: eventPluginsManager
            Component.onCompleted: {
                populateEnabledPluginsList(Plasmoid.configuration.enabledCalendarPlugins);
            }
        }

        QQC2.CheckBox {
            id: showWeekNumbers
            Kirigami.FormData.label: i18n("General:")
            text: i18n("Show week numbers")
        }


        QQC2.ComboBox {
            id: firstDayOfWeekCombo

            Kirigami.FormData.label: i18n("First day of week:")
            Layout.fillWidth: true

            textRole: "text"
            model: [-1, 0, 1, 5, 6].map(day => ({
                day,
                text: day === -1 ? i18n("Use Region Defaults") : Qt.locale().dayName(day),
            }))
            onActivated: index => {
                cfg_firstDayOfWeek = model[index].day;
            }
            currentIndex: model.findIndex(item => item.day === cfg_firstDayOfWeek)
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        ColumnLayout {
            id: calendarPluginsLayout

            spacing: Kirigami.Units.smallSpacing

            Kirigami.FormData.label: i18n("Available Plugins:")

            Repeater {
                id: calendarPluginsRepeater

                model: eventPluginsManager.model

                delegate: QQC2.CheckBox {
                    required property var model

                    text: model.display
                    checked: model.checked

                    Accessible.onPressAction: {
                        toggle();
                        clicked();
                    }

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
