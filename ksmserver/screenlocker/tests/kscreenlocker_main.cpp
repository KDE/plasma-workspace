/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>

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
#include "../ksldapp.h"
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral(
        "This test application starts the screen locker immediatelly and\n"
        "exits once the screen got successfully unlocked. The purpose is\n"
        "to test changes in KSLD without having to restart KSMServer.\n"
        "Thus it's a good way to verify grabbing of keyboard/pointer and\n"
        "the communication with kscreenlocker_greet. If the lock is not\n"
        "working properly the test application can be killed and the\n"
        "screen is unlocked again. If one just wants to test the greeter\n"
        "it's better to start just kscreenlocker_greet."));

    parser.addHelpOption();

    parser.process(app);

    ScreenLocker::KSldApp locker(&app);
    QObject::connect(&locker, &ScreenLocker::KSldApp::unlocked, &app, &QApplication::quit);
    locker.lock(true);

    return app.exec();
}
