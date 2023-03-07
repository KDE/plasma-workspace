/*
    SPDX-FileCopyrightText: 2007 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2008 Alex Merry <alex.merry@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "timeengine.h"

#include <QDBusConnection>
#include <QDate>
#include <QSocketNotifier>
#include <QStringList>
#include <QTime>

#ifdef Q_OS_LINUX
#include <fcntl.h>
#include <sys/timerfd.h>
#include <unistd.h>
#endif

#include "debug.h"
#include "timesource.h"

// timezone is defined in msvc
#ifdef timezone
#undef timezone
#endif

TimeEngine::TimeEngine(QObject *parent, const QVariantList &args)
    : Plasma5Support::DataEngine(parent, args)
{
    Q_UNUSED(args)
    setMinimumPollingInterval(333);

    // To have translated timezone names
    // (effectively a noop if the catalog is already present).
    ////KF5 port: remove this line and define TRANSLATION_DOMAIN in CMakeLists.txt instead
    // KLocale::global()->insertCatalog("timezones4");
    QTimer::singleShot(0, this, &TimeEngine::init);
}

TimeEngine::~TimeEngine()
{
}

void TimeEngine::init()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.connect(QString(), QString(), QStringLiteral("org.kde.KTimeZoned"), QStringLiteral("timeZoneChanged"), this, SLOT(tzConfigChanged()));

#ifdef Q_OS_LINUX
    // monitor for the system clock being changed
    auto timeChangedFd = timerfd_create(CLOCK_REALTIME, O_CLOEXEC | O_NONBLOCK);
    itimerspec timespec;
    memset(&timespec, 0, sizeof(timespec)); // set all timers to 0 seconds, which creates a timer that won't do anything

    int err = timerfd_settime(timeChangedFd, 3, &timespec, nullptr); // monitor for the time changing
    //(flags == TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET). However these are not exposed in glibc so value is hardcoded
    if (err) {
        qCWarning(DATAENGINE_TIME) << "Could not create timer with TFD_TIMER_CANCEL_ON_SET. Clock skews will not be detected. Error:"
                                   << qPrintable(strerror(err));
    }

    connect(this, &QObject::destroyed, [timeChangedFd]() {
        close(timeChangedFd);
    });

    auto notifier = new QSocketNotifier(timeChangedFd, QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, [this](int fd) {
        uint64_t c;
        read(fd, &c, 8);
        clockSkewed();
    });
#else
    dbus.connect(QString(), "/org/kde/kcmshell_clock", "org.kde.kcmshell_clock", "clockUpdated", this, SLOT(clockSkewed()));
    dbus.connect(QStringLiteral("org.kde.Solid.PowerManagement"),
                 QStringLiteral("/org/kde/Solid/PowerManagement/Actions/SuspendSession"),
                 QStringLiteral("org.kde.Solid.PowerManagement.Actions.SuspendSession"),
                 QStringLiteral("resumingFromSuspend"),
                 this,
                 SLOT(clockSkewed()));
#endif
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
    TimeSource *s = qobject_cast<TimeSource *>(containerForSource(QStringLiteral("Local")));

    if (s) {
        s->setTimeZone(QStringLiteral("Local"));
    }

    updateAllSources();
    forceImmediateUpdateOfAllVisualizations();
}

QStringList TimeEngine::sources() const
{
    QStringList sources;
    Q_FOREACH (const QByteArray &tz, QTimeZone::availableTimeZoneIds()) {
        sources << QString(tz.constData());
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
    TimeSource *s = qobject_cast<TimeSource *>(containerForSource(tz));

    if (s) {
        s->updateTime();
        return true;
    }

    return false;
}

K_PLUGIN_CLASS_WITH_JSON(TimeEngine, "plasma-dataengine-time.json")

#include "timeengine.moc"
