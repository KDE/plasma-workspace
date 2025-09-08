/*
    SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "debug.h"
#include "startplasma.h"
#include <KConfig>
#include <KConfigGroup>
#include <QDBusConnection>
#include <QDBusInterface>
#include <signal.h>

static void setupWorkarounds()
{
#if WITH_X11
    // Tell AWT that kwin_wayland is a non-reparenting window manager. Without this, AWT will ignore
    // ConfigureNotify events and java applications will paint nothing on the screen.
    qputenv("_JAVA_AWT_WM_NONREPARENTING", "1");
#endif
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    createConfigDirectory();
    setupCursor(true);
    setupWorkarounds();
    signal(SIGTERM, sigtermHandler);

    // Let clients try to reconnect to kwin after a restart
    qputenv("QT_WAYLAND_RECONNECT", "1");

    // Query whether org.freedesktop.locale1 is available. If it is, try to
    // set XKB_DEFAULT_{MODEL,LAYOUT,VARIANT,OPTIONS} accordingly.
    {
        const QString locale1Service = QStringLiteral("org.freedesktop.locale1");
        const QString locale1Path = QStringLiteral("/org/freedesktop/locale1");
        QDBusMessage message =
            QDBusMessage::createMethodCall(locale1Service, locale1Path, QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("GetAll"));
        message << locale1Service;
        QDBusMessage resultMessage = QDBusConnection::systemBus().call(message);
        if (resultMessage.type() == QDBusMessage::ReplyMessage) {
            QVariantMap result;
            auto dbusArgument = resultMessage.arguments().at(0).value<QDBusArgument>();
            while (!dbusArgument.atEnd()) {
                dbusArgument >> result;
            }

            auto queryAndSet = [&result](const char *var, const QString &value) {
                const auto r = result.value(value).toString();
                if (!r.isEmpty())
                    qputenv(var, r.toUtf8());
            };

            queryAndSet("XKB_DEFAULT_MODEL", QStringLiteral("X11Model"));
            queryAndSet("XKB_DEFAULT_LAYOUT", QStringLiteral("X11Layout"));
            queryAndSet("XKB_DEFAULT_VARIANT", QStringLiteral("X11Variant"));
            queryAndSet("XKB_DEFAULT_OPTIONS", QStringLiteral("X11Options"));
        } else {
            qCWarning(PLASMA_STARTUP) << "not a reply org.freedesktop.locale1" << resultMessage;
        }
    }
    runEnvironmentScripts();

    if (!qEnvironmentVariableIsSet("DBUS_SESSION_BUS_ADDRESS")) {
        out << "startplasmacompositor: Could not start D-Bus. Can you call qdbus?\n";
        return 1;
    }
    setupPlasmaEnvironment();
    runStartupConfig();
    qputenv("XDG_SESSION_TYPE", "wayland");

    auto oldSystemdEnvironment = getSystemdEnvironment();
    if (!syncDBusEnvironment()) {
        out << "Could not sync environment to dbus.\n";
        return 1;
    }

    // We import systemd environment after we sync the dbus environment here.
    // Otherwise it may leads to some unwanted order of applying environment
    // variables (e.g. LANG and LC_*)
    importSystemdEnvrionment();

    if (!startPlasmaSession(true))
        return 4;

    // Anything after here is logout
    // It is not called after shutdown/restart
    waitForKonqi();
    out << "startplasma-wayland: Shutting down...\n";
    out << "startplasmacompositor: Shutting down...\n";
    stopSystemdSession();
    cleanupPlasmaEnvironment(oldSystemdEnvironment);
    out << "startplasmacompositor: Done.\n";

    return 0;
}
