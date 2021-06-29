/*
    SPDX-FileCopyrightText: 2014 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "digitalclockplugin.h"
#include "applicationintegration.h"
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

    qmlRegisterSingletonType<ApplicationIntegration>(uri, 1, 0, "ApplicationIntegration", [](QQmlEngine *engine, QJSEngine *scriptEngine) {
        Q_UNUSED(engine);
        Q_UNUSED(scriptEngine);
        return new ApplicationIntegration();
    });
}
