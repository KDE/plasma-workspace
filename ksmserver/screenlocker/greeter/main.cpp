/********************************************************************
 This file is part of the KDE project.

Copyright (C) 2011 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include <KLocalizedString>

#include <QDateTime>
#include <QCommandLineParser>
#include <QSessionManager>

#include <iostream>

#include <signal.h>

#include "greeterapp.h"

static void signalHandler(int signum)
{
    ScreenLocker::UnlockApp *instance = qobject_cast<ScreenLocker::UnlockApp *>(QCoreApplication::instance());

    if (!instance)
        return;

    switch(signum)
    {
      case SIGTERM:
        // exit gracefully to not leave behind screensaver processes (bug#224200)
        // return exit code 1 to indicate that a valid password was not entered,
        // to prevent circumventing the password input by sending a SIGTERM
        instance->exit(1);
        break;
      case SIGUSR1:
        instance->lockImmediately();
        break;
    }
}

int main(int argc, char* argv[])
{
    KLocalizedString::setApplicationDomain("kscreenlocker_greet");

    ScreenLocker::UnlockApp app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("kscreenlocker_greet"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));

    // disable session management for the greeter
    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("Greeter for the KDE Plasma Workspaces Screen locker"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption testingOption(QStringLiteral("testing"),
                                     i18n("Starts the greeter in testing mode"));
    QCommandLineOption immediateLockOption(QStringLiteral("immediateLock"),
                                           i18n("Lock immediately, ignoring any grace time etc."));
    QCommandLineOption graceTimeOption(QStringLiteral("graceTime"),
                                       i18n("Delay till the lock user interface gets shown in milliseconds."),
                                       QStringLiteral("milliseconds"),
                                       QStringLiteral("0"));
    QCommandLineOption nolockOption(QStringLiteral("nolock"),
                                    i18n("Don't show any lock user interface."));

    parser.addOption(testingOption);
    parser.addOption(immediateLockOption);
    parser.addOption(graceTimeOption);
    parser.addOption(nolockOption);
    parser.process(app);

    if (parser.isSet(testingOption)) {
        app.setTesting(true);
        app.setImmediateLock(true);
    } else {
        app.setImmediateLock(parser.isSet(immediateLockOption));
    }
    app.setNoLock(parser.isSet(nolockOption));
    bool ok = false;
    int graceTime = parser.value(graceTimeOption).toInt(&ok);
    if (ok) {
        app.setGraceTime(graceTime);
    }
    app.desktopResized();

    // This allow ksmserver to know when the applicaion has actually finished setting itself up.
    // Crucial for blocking until it is ready, ensuring locking happens before sleep, e.g.
    std::cout << "Locked at " << QDateTime::currentDateTime().toTime_t() << std::endl;

    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGUSR1, &sa, nullptr);
    return app.exec();
}
