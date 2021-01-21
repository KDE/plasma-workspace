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

#include <KConfig>
#include <KConfigGroup>
#include <signal.h>
#include <unistd.h>

void sighupHandler(int)
{
    out << "GOT SIGHUP\n";
}

int main(int argc, char **argv)
{
    // When the X server dies we get a HUP signal from xinit. We must ignore it
    // because we still need to do some cleanup.
    signal(SIGHUP, sighupHandler);

    QCoreApplication app(argc, argv);

    // Boot sequence:
    //
    // kdeinit is used to fork off processes which improves memory usage
    // and startup time.
    //
    // * kdeinit starts klauncher first.
    // * Then kded is started. kded is responsible for keeping the sycoca
    // database up to date. When an up to date database is present it goes
    // into the background and the startup continues.
    // * Then kdeinit starts kcminit. kcminit performs initialisation of
    // certain devices according to the user's settings
    //
    // * Then ksmserver is started which takes control of the rest of the startup sequence

    // Check if a Plasma session already is running and whether it's possible to connect to X
    switch (kCheckRunning()) {
    case NoX11:
        out << "$DISPLAY is not set or cannot connect to the X server.\n";
        return 1;
    case PlasmaRunning:
        messageBox(QStringLiteral("Plasma seems to be already running on this display.\n"));
        return 1;
    case NoPlasmaRunning:
        break;
    }

    createConfigDirectory();
    runStartupConfig();

    // Do not sync any of this section with the wayland versions as there scale factors are
    // sent properly over wl_output

    {
        KConfig cfg(QStringLiteral("kdeglobals"));

        KConfigGroup kscreenGroup = cfg.group("KScreen");
        const auto screenScaleFactors = kscreenGroup.readEntry("ScreenScaleFactors", QByteArray());
        if (!screenScaleFactors.isEmpty()) {
            qputenv("QT_SCREEN_SCALE_FACTORS", screenScaleFactors);
            qreal scaleFactor = qFloor(kscreenGroup.readEntry("ScaleFactor", 1.0));
            if (scaleFactor > 1) {
                qputenv("GDK_SCALE", QByteArray::number(scaleFactor, 'g', 0));
                qputenv("GDK_DPI_SCALE", QByteArray::number(1.0 / scaleFactor, 'g', 3));
            }
        }
    }

    setupCursor(false);
    setupFontDpi();
    QScopedPointer<QProcess, KillBeforeDeleter> ksplash(setupKSplash());

    runEnvironmentScripts();

    out << "startkde: Starting up...\n";

    setupPlasmaEnvironment();
    setupX11();

    if (!syncDBusEnvironment()) {
        // Startup error
        messageBox(QStringLiteral("Could not sync environment to dbus.\n"));
        return 1;
    }

    if (!startPlasmaSession(false))
        return 1;

    // Anything after here is logout
    // It is not called after shutdown/restart
    waitForKonqi();

    out << "startkde: Shutting down...\n";

    runSync(QStringLiteral("kdeinit5_shutdown"), {});

    cleanupPlasmaEnvironment();
    cleanupX11();

    out << "startkde: Done.\n";

    return 0;
}
