/* This file is part of the KDE project
   Copyright (C) 2019 Aleix Pol Gonzalez <aleixpol@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "startplasma.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    setupFontDpi();
    QScopedPointer<QProcess, KillBeforeDeleter> ksplash;

    if (!qEnvironmentVariableIsSet("KWIN_RESTART_COUNT")) {
        ksplash.reset(setupKSplash());
    }

    out << "startplasma-waylandsession: Starting up...";

    if (qEnvironmentVariableIsSet("DISPLAY")) {
        setupX11();
    } else {
        qWarning() << "running kwin without Xwayland support";
    }

    if (!syncDBusEnvironment()) {
        out << "Could not sync environment to dbus.\n";
        return 2;
    }

    if (!startPlasmaSession(true))
        return 4;

    // Anything after here is logout
    // It is not called after shutdown/restart
    waitForKonqi();
    out << "startplasma-waylandsession: Shutting down...\n";

    // Keep for KF5; remove in KF6 (KInit will be gone then)
    runSync(QStringLiteral("kdeinit5_shutdown"), {});

    cleanupX11();
    out << "startplasma-waylandsession: Done.\n";

    return 0;
}
