/*
    SPDX-FileCopyrightText: 2007 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2008 Alex Merry <alex.merry@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "timeengine.h"

#include <KSystemClockSkewNotifier>

#include <QDBusConnection>
#include <QDate>
#include <QStringList>
#include <QTime>

#include "debug.h"
#include "timesource.h"

// timezone is defined in msvc
#ifdef timezone
#undef timezone
#endif

TimeEngine::TimeEngine(QObject *parent)
    : Plasma5Support::DataEngine(parent)
{
    setMinimumPollingInterval(333);

    // To have translated timezone names
    // (effectively a noop if the catalog is already present).
    ////KF5 port: remove this line and define TRANSLATION_DOMAIN in CMakeLists.txt instead
    // KLocale::global()->insertCatalog("timezones4");
    QTimer::singleShot(0, this, &TimeEngine::init);
}

TimeEngine::~TimeEngine() = default;

void TimeEngine::init()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.connect(QString(), QString(), QStringLiteral("org.kde.KTimeZoned"), QStringLiteral("timeZoneChanged"), this, SLOT(tzConfigChanged()));

    auto notifier = new KSystemClockSkewNotifier(this);
    connect(notifier, &KSystemClockSkewNotifier::skewed, this, &TimeEngine::clockSkewed);
    notifier->setActive(true);
}

void TimeEngine::clockSkewed()
{
    qCDebug(DATAENGINE_TIME) << "Time engine Clock skew signaled";
    updateAllSources();
    forceImmediateUpdateOfAllVisualizations();
}

void TimeEngine::tzConfigChanged()
{
    qCDebug(DATAENGINE_TIME) << "Local timezone changed signaled";
    auto *s = qobject_cast<TimeSource *>(containerForSource(QStringLiteral("Local")));

    if (s) {
        s->setTimeZone(QStringLiteral("Local"));
    }

    updateAllSources();
    forceImmediateUpdateOfAllVisualizations();
}

QStringList TimeEngine::sources() const
{
    QStringList sources;
    for (const auto timezones = QTimeZone::availableTimeZoneIds(); const QByteArray &tz : timezones) {
        sources << QString::fromLocal8Bit(tz);
    }
    sources << QStringLiteral("Local");
    return sources;
}

bool TimeEngine::sourceRequestEvent(const QString &name)
{
    addSource(new TimeSource(name, this));
    return true;
}

bool TimeEngine::updateSourceEvent(const QString &tz)
{
    auto *s = qobject_cast<TimeSource *>(containerForSource(tz));

    if (s) {
        s->updateTime();
        return true;
    }

    return false;
}

K_PLUGIN_CLASS_WITH_JSON(TimeEngine, "plasma-dataengine-time.json")

#include "timeengine.moc"

#include "moc_timeengine.cpp"
