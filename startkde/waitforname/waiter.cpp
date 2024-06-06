/*
    SPDX-FileCopyrightText: 2017 Valerio Pilo <vpilo@coldshock.net>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include <QCommandLineParser>
#include <QDebug>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>

#include <unistd.h>

#include "debug_p.h"
#include "waiter.h"

constexpr static const char dbusServiceName[] = "org.freedesktop.Notifications";

Waiter::Waiter(int argc, char **argv)
    : QCoreApplication(argc, argv)
    , mService(dbusServiceName)
{
    setApplicationName(QStringLiteral("plasma_waitforname"));
    setApplicationVersion(QStringLiteral("1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Waits for D-Bus registration of the Notifications service.\n"
                       "Prevents notifications from being processed before the desktop is ready."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("service"),
                                 QStringLiteral("Optionally listen for a service different than '%1'").arg(mService),
                                 QStringLiteral("[service]"));

    QCommandLineOption timeoutOption(QStringList() << QStringLiteral("timeout"),
                                     QStringLiteral("Timeout in seconds. Use -1 to disable"),
                                     QStringLiteral("seconds"),
                                     QString::number(dbusTimeoutSec));

    QCommandLineOption unregisterModeOption(QStringList() << QStringLiteral("unregistered"),
                                            QStringLiteral("Wait for service unregistration, instead of registration"));

    parser.addOption(timeoutOption);
    parser.addOption(unregisterModeOption);
    parser.process(*this);

    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        mService = args.at(0);
    } else {
        exit(-1);
    }

    bool intIsOk;
    int timeout = parser.value(timeoutOption).toInt(&intIsOk);
    if (!intIsOk) {
        qCWarning(LOG_PLASMA) << "Invalid timeout value";
        exit(-1);
    }
    if (timeout > 0) {
        mTimeoutTimer.setSingleShot(true);
        mTimeoutTimer.setInterval(timeout * 1000);
        connect(&mTimeoutTimer, &QTimer::timeout, this, &Waiter::timeout);
        mTimeoutTimer.start();
    }

    if (parser.isSet(unregisterModeOption)) {
        mMode = WaitForUnregistration;
    }
}

bool Waiter::waitForService()
{
    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    QDBusServiceWatcher *watcher = new QDBusServiceWatcher(this);
    watcher->setConnection(sessionBus);
    watcher->addWatchedService(mService);

    if (mMode == Waiter::WaitForRegistration) {
        if (sessionBus.interface()->isServiceRegistered(mService)) {
            qCDebug(LOG_PLASMA) << "WaitForName: Service" << mService << "is already registered";
            return false;
        }

        qCDebug(LOG_PLASMA) << "WaitForName: Waiting for appearance of service" << mService << "for" << dbusTimeoutSec << "seconds";
        watcher->setWatchMode(QDBusServiceWatcher::WatchForRegistration);
        connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, &Waiter::registered);
    } else {
        if (!sessionBus.interface()->isServiceRegistered(mService)) {
            qCDebug(LOG_PLASMA) << "WaitForName: Service" << mService << "is not yet registered";
            return false;
        }
        watcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
        connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, &Waiter::unregistered);
    }
    return true;
}

void Waiter::registered()
{
    qCDebug(LOG_PLASMA) << "WaitForName: Service was registered after" << (mTimeoutTimer.interval() - (mTimeoutTimer.remainingTime() / 1000)) << "seconds";
    mTimeoutTimer.stop();
    exit(0);
}

void Waiter::unregistered()
{
    qCDebug(LOG_PLASMA) << "WaitForName: Service was unregistered after" << (mTimeoutTimer.interval() - (mTimeoutTimer.remainingTime() / 1000)) << "seconds";
    mTimeoutTimer.stop();
    exit(0);
}

void Waiter::timeout()
{
    qCInfo(LOG_PLASMA) << "WaitForName: Service was not registered within timeout";
    exit(1);
}
