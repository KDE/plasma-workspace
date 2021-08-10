/*
    SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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

    out << "startplasma-waylandsession: Done.\n";

    return 0;
}
