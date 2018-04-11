/*
    Copyright (C) 2014  Martin Klapetek <mklapetek@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "digitalclockplugin.h"
#include "clipboardmenu.h"
#include "timezonemodel.h"
#include "timezonesi18n.h"

#include <QQmlEngine>

static QObject *timezonesi18n_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return new TimezonesI18n();
}

static QObject *clipboardMenu_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine);
    Q_UNUSED(scriptEngine);

    return new ClipboardMenu();
}

void DigitalClockPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("org.kde.plasma.private.digitalclock"));

    qmlRegisterType<TimeZoneModel>(uri, 1, 0, "TimeZoneModel");
    qmlRegisterType<TimeZoneFilterProxy>(uri, 1, 0, "TimeZoneFilterProxy");
    qmlRegisterSingletonType<TimezonesI18n>(uri, 1, 0, "TimezonesI18n", timezonesi18n_singletontype_provider);

    qmlRegisterSingletonType<ClipboardMenu>(uri, 1, 0, "ClipboardMenu", clipboardMenu_singletontype_provider);
}
