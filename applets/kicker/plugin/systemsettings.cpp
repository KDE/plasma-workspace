/*
    SPDX-FileCopyrightText: 2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "systemsettings.h"

#include <QStandardPaths>

SystemSettings::SystemSettings(QObject *parent)
    : QObject(parent)
{
}

SystemSettings::~SystemSettings()
{
}

QString SystemSettings::picturesLocation() const
{
    QString path;

    const QStringList &locations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);

    if (!locations.isEmpty()) {
        path = locations.at(0);
    } else {
        // HomeLocation is guaranteed not to be empty.
        path = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
    }

    return path;
}
