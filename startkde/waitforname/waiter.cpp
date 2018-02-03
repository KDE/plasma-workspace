/*
 *   Copyright © 2017 Valerio Pilo <vpilo@coldshock.net>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QCommandLineParser>
#include <QDebug>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>

#include <unistd.h>

#include "waiter.h"
#include "debug_p.h"


constexpr static const char dbusServiceName[] = "org.freedesktop.Notifications";


Waiter::Waiter(int argc, char **argv)
    : QCoreApplication(argc, argv)
    , mService(dbusServiceName) {
    setApplicationName(QStringLiteral("plasma_waitforname"));
    setApplicationVersion(QStringLiteral("1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Waits for D-Bus registration of the Notifications service.\n"
        "Prevents notifications from being processed before the desktop is ready."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("service"),
                                 QStringLiteral("Optionally listen for a service different than '%1'").arg(mService),
                                 QStringLiteral("[service]"));
    parser.process(*this);

    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        mService = args.at(0);
    }
}

bool Waiter::waitForService() {
    QDBusConnection sessionBus = QDBusConnection::sessionBus();

    if (sessionBus.interface()->isServiceRegistered(mService)) {
        qCDebug(LOG_PLASMA) << "WaitForName: Service" << mService << "is already registered";
        return false;
    }

    qCDebug(LOG_PLASMA) << "WaitForName: Waiting for appearance of service" << mService << "for" << dbusTimeoutSec << "seconds";

    QDBusServiceWatcher* watcher = new QDBusServiceWatcher(this);
    watcher->setConnection(sessionBus);
    watcher->setWatchMode(QDBusServiceWatcher::WatchForRegistration);
    watcher->addWatchedService(mService);
    connect(watcher, &QDBusServiceWatcher::serviceRegistered,
            this,    &Waiter::registered);

    mTimeoutTimer.setSingleShot(true);
    mTimeoutTimer.setInterval(dbusTimeoutSec * 1000);
    connect(&mTimeoutTimer, &QTimer::timeout, this, &Waiter::timeout);
    mTimeoutTimer.start();

    return true;
}

void Waiter::registered() {
    qCDebug(LOG_PLASMA) << "WaitForName: Service was registered after" << (dbusTimeoutSec - (mTimeoutTimer.remainingTime() / 1000)) << "seconds";
    mTimeoutTimer.stop();
    exit(0);
}

void Waiter::timeout() {
    qCInfo(LOG_PLASMA) << "WaitForName: Service was not registered within timeout";
    exit(1);
}
