/*
    SPDX-FileCopyrightText: 2016 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.0

import org.kde.plasma.configuration 2.0

ConfigModel {
    ConfigCategory {
         name: i18n("Appearance")
         icon: "preferences-desktop-color"
         source: "configGeneral.qml"
    }
}
