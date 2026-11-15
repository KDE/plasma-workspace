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

KCMUtils.ScrollViewKCM {
    id: calendarPage

    signal configurationChanged()

    property alias cfg_showWeekNumbers: showWeekNumbers.checked
    property int cfg_firstDayOfWeek
    property bool unsavedChanges: false

    extraFooterTopPadding: true

    function saveConfig() {
        Plasmoid.configuration.enabledCalendarPlugins = eventPluginsManager.enabledPlugins;
        Plasmoid.configuration.writeConfig()
        unsavedChanges = false
    }

    function checkUnsavedChanges() {
            calendarPage.unsavedChanges = !(Plasmoid.configuration.enabledCalendarPlugins.every(entry => eventPluginsManager.enabledPlugins.includes(entry)) &&  eventPluginsManager.enabledPlugins.every(entry =>  Plasmoid.configuration.enabledCalendarPlugins.includes(entry)))
    }

    header: Kirigami.Form {
        PlasmaCalendar.EventPluginsManager {
            id: eventPluginsManager
            Component.onCompleted: {
                populateEnabledPluginsList(Plasmoid.configuration.enabledCalendarPlugins);
            }
        }

        Kirigami.FormGroup {
            Kirigami.FormEntry {
                title: i18nc("@option:check formdata label", "General:")
                contentItem: QQC2.CheckBox {
                    id: showWeekNumbers
                    text: i18nc("@option:check", "Show week numbers")
                }
            }

            Kirigami.FormEntry {
                title: i18nc("@label:listbox", "First day of week:")
                contentItem: QQC2.ComboBox {
                    id: firstDayOfWeekCombo

                    Layout.fillWidth: true

                    textRole: "text"
                    model: [-1, 0, 1, 5, 6].map(day => ({
                        day,
                        text: day === -1 ? i18nc("@item:inlistbox first day of week option", "Use region defaults") : Qt.locale().dayName(day),
                    }))
                    onActivated: index => {
                        calendarPage.cfg_firstDayOfWeek = model[index].day;
                    }
                    currentIndex: model.findIndex(item => item.day === calendarPage.cfg_firstDayOfWeek)
                }
            }
        }
    }

    view: ListView {
        id: pluginListView
        activeFocusOnTab: true
        model: eventPluginsManager.model
        header: Kirigami.InlineViewHeader {
                text: i18nc("@title:column", "Available Add-Ons")
                width: pluginListView.width
        }
        headerPositioning: ListView.OverlayHeader
        delegate: QQC2.CheckDelegate {
            id: delegate
            required property var model
            width: pluginListView.width
            checked: model.checked
            text: model.display
            property string subtitle: model.toolTip
            icon.source: model.decoration

            Accessible.onPressAction: {
                toggle();
                clicked();
            }

            onClicked: {
                //needed for model's setData to be called
                model.checked = checked;
                calendarPage.checkUnsavedChanges();
            }

            contentItem: Kirigami.IconTitleSubtitle {
                width: delegate.availableWidth
                icon: icon.fromControlsIcon(delegate.icon)
                title: delegate.text
                subtitle: delegate.subtitle
                selected: delegate.highlighted || delegate.pressed
            }
        }
    }
}
