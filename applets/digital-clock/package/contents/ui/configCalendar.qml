/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.0
import QtQuick.Controls 2.4 as QtControls
import QtQuick.Layouts 1.0 as QtLayouts
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.workspace.calendar 2.0 as PlasmaCalendar
import org.kde.kirigami 2.5 as Kirigami

Item {
    id: calendarPage
    width: childrenRect.width
    height: childrenRect.height

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
        anchors {
            left: parent.left
            right: parent.right
        }

        QtControls.CheckBox {
            id: showWeekNumbers
            Kirigami.FormData.label: i18n("General:")
            text: i18n("Show week numbers")
        }

        QtLayouts.RowLayout {
            QtLayouts.Layout.fillWidth: true
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

        QtLayouts.ColumnLayout {
            Kirigami.FormData.label: i18n("Available Plugins:")
            Kirigami.FormData.buddyFor: children[1] // 0 is the Repeater

            Repeater {
                id: calendarPluginsRepeater
                model: eventPluginsManager.model
                delegate: QtLayouts.RowLayout {
                    QtControls.CheckBox {
                        text: model.display
                        checked: model.checked
                        onClicked: {
                            //needed for model's setData to be called
                            model.checked = checked;
                            calendarPage.configurationChanged();
                        }
                    }
                }
            }
        }
    }
}

