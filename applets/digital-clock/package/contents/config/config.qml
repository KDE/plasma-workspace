/*
    SPDX-FileCopyrightText: 2013 Bhushan Shah <bhush94@gmail.com>
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

pragma ComponentBehavior: Bound

import QtQuick

import org.kde.plasma.plasmoid
import org.kde.plasma.configuration
import org.kde.plasma.workspace.calendar as PlasmaCalendar

ConfigModel {
    id: configModel

    ConfigCategory {
         name: i18n("Appearance")
         icon: "preferences-desktop-color"
         source: "configAppearance.qml"
    }
    ConfigCategory {
        name: i18n("Calendar")
        icon: "office-calendar"
        source: "configCalendar.qml"
    }
    ConfigCategory {
        name: i18n("Time Zones")
        icon: "preferences-system-time"
        source: "configTimeZones.qml"
        includeMargins: false
    }

    readonly property PlasmaCalendar.EventPluginsManager eventPluginsManager: PlasmaCalendar.EventPluginsManager {
        Component.onCompleted: {
            populateEnabledPluginsList(Plasmoid.configuration.enabledCalendarPlugins);
        }
    }

    readonly property Instantiator __eventPlugins: Instantiator {
        model: configModel.eventPluginsManager.model
        delegate: ConfigCategory {
            required property string display
            required property string decoration
            required property string configUi
            required property string pluginId

            name: display
            icon: decoration
            source: configUi
            includeMargins: false
            visible: Plasmoid.configuration.enabledCalendarPlugins.indexOf(pluginId) > -1
        }

        onObjectAdded: (index, object) => configModel.appendCategory(object)
        onObjectRemoved: (index, object) => configModel.removeCategory(object)
    }
}
