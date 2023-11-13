/*
    SPDX-FileCopyrightText: 2015 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: MIT
*/

#include "sessionsprivateplugin.h"

#include <QQmlEngine>

#include <sessionmanagement.h>
#include <sessionsmodel.h>

void SessionsPrivatePlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("org.kde.plasma.private.sessions"));

    qmlRegisterType<SessionManagement>(uri, 2, 0, "SessionManagement");
    qmlRegisterType<SessionsModel>(uri, 2, 0, "SessionsModel");
}
