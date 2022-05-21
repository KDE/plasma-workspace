/*
    SPDX-FileCopyrightText: 2013 Mark Gaiser <markg85@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "calendarplugin.h"
#include "calendar.h"
#include "eventdatadecorator.h"
#include "eventpluginsmanager.h"

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QtQml>

void CalendarPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QByteArray("org.kde.plasma.workspace.calendar"));
    qmlRegisterType<Calendar>(uri, 2, 0, "Calendar");
    qmlRegisterType<EventPluginsManager>(uri, 2, 0, "EventPluginsManager");
    qmlRegisterAnonymousType<QAbstractItemModel>(uri, 1);
    qmlRegisterAnonymousType<QAbstractListModel>(uri, 1);
    qmlRegisterUncreatableType<EventDataDecorator>(uri, 2, 0, "EventDataDecorator", QStringLiteral("Unable to create EventDataDecorator from QML"));
}
