/*
    SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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

    runEnvironmentScripts();

    out << "startkde: Starting up...\n";

    setupPlasmaEnvironment();
    setupX11();

    auto oldSystemdEnvironment = getSystemdEnvironment();
    if (!syncDBusEnvironment()) {
        // Startup error
        messageBox(QStringLiteral("Could not sync environment to dbus.\n"));
        return 1;
    }

    // We import systemd environment after we sync the dbus environment here.
    // Otherwise it may leads to some unwanted order of applying environment
    // variables (e.g. LANG and LC_*)
    importSystemdEnvrionment();

    // NOTE: Do not start QGuiApplications before setting up the environment. We'd be at risk of dbus invoking other
    // processes with an incomplete environment.
    setupCursor(false);
    std::unique_ptr<QProcess, KillBeforeDeleter> ksplash(setupKSplash());

    if (!startPlasmaSession(false))
        return 1;

    // Anything after here is logout
    // It is not called after shutdown/restart
    waitForKonqi();

    out << "startkde: Shutting down...\n";

    // Keep for KF5; remove in KF6 (KInit will be gone then)
    runSync(QStringLiteral("kdeinit5_shutdown"), {});

    cleanupPlasmaEnvironment(oldSystemdEnvironment);

    out << "startkde: Done.\n";

    return 0;
}
