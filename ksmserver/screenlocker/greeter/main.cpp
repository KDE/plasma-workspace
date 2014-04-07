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

#include <iostream>

#include "greeterapp.h"

int main(int argc, char* argv[])
{
    ScreenLocker::UnlockApp app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("kscreenlocker_greet"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("Greeter for the KDE Plasma Workspaces Screen locker"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption testingOption(QStringLiteral("testing"),
                                     i18n("Starts the greeter in testing mode"));
    QCommandLineOption immediateLockOption(QStringLiteral("immediateLock"),
                                           i18n("Lock immediately, ignoring any grace time etc."));

    parser.addOption(testingOption);
    parser.addOption(immediateLockOption);
    parser.process(app);

    if (parser.isSet(testingOption)) {
        app.setTesting(true);
        app.setImmediateLock(true);
    } else {
        app.setImmediateLock(parser.isSet(immediateLockOption));
    }
    app.desktopResized();

    // This allow ksmserver to know when the applicaion has actually finished setting itself up.
    // Crucial for blocking until it is ready, ensuring locking happens before sleep, e.g.
    std::cout << "Locked at " << QDateTime::currentDateTime().toTime_t() << std::endl;
    return app.exec();
}
