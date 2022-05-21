/*
    SPDX-FileCopyrightText: 2013 Bhushan Shah <bhush94@gmail.com>
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.0
import QtQml 2.2

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.configuration 2.0
import org.kde.plasma.workspace.calendar 2.0 as PlasmaCalendar

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
    }

    property QtObject eventPluginsManager: PlasmaCalendar.EventPluginsManager {
        Component.onCompleted: {
            populateEnabledPluginsList(Plasmoid.configuration.enabledCalendarPlugins);
        }
    }

    property Instantiator __eventPlugins: Instantiator {
        model: eventPluginsManager.model
        delegate: ConfigCategory {
            name: model.display
            icon: model.decoration
            source: model.configUi
            visible: Plasmoid.configuration.enabledCalendarPlugins.indexOf(model.pluginPath) > -1
        }

        onObjectAdded: configModel.appendCategory(object)
        onObjectRemoved: configModel.removeCategory(object)
    }
}
