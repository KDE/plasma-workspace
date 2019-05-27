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
#include <KSharedConfig>
#include <KConfigGroup>
#include <QDBusConnection>
#include <QDBusInterface>

int main(int /*argc*/, char** /*argv*/)
{
    if (!createStartupConfig())
        return 1;

    setupCursor(true);

    {
        KSharedConfig::Ptr fonts = KSharedConfig::openConfig("kcmfonts");
        KConfigGroup group(fonts, "General");
        auto dpiSetting = group.readEntry("forceFontDPIWayland", 96);
        auto dpi = dpiSetting == 0 ? 96 : dpiSetting;
        qputenv("QT_WAYLAND_FORCE_DPI", QByteArray::number(dpi));
    }

    // Query whether org.freedesktop.locale1 is available. If it is, try to
    // set XKB_DEFAULT_{MODEL,LAYOUT,VARIANT,OPTIONS} accordingly.
    if (QDBusConnection::systemBus().interface()->isServiceRegistered("org.freedesktop.locale1")) {
        auto queryAndSet = [](const QByteArray &var, const QByteArray & value) {
            QDBusInterface iface(QStringLiteral("org.freedesktop.locale1"), QStringLiteral("/org/freedesktop/locale1"), QStringLiteral("org.freedesktop.locale1"), QDBusConnection::systemBus());

            QString r = iface.property(value).toString();
            if (!r.isEmpty())
                qputenv(var, r.toLatin1());
        };

        queryAndSet("X11MODEL", "org.freedesktop.locale1.X11Model");
        queryAndSet("X11LAYOUT", "org.freedesktop.locale1.X11Layout");
        queryAndSet("X11VARIANT", "org.freedesktop.locale1.X11Variant");
        queryAndSet("X11OPTIONS", "org.freedesktop.locale1.X11Options");
    }

    runEnvironmentScripts();

    if (!qEnvironmentVariableIsSet("DBUS_SESSION_BUS_ADDRESS")) {
        out << "startplasmacompositor: Could not start D-Bus. Can you call qdbus?\n";
        return 1;
    }
    setupPlasmaEnvironment();
    qputenv("XDG_SESSION_TYPE", "wayland");

    if (!syncDBusEnvironment()) {
        out << "Could not sync environment to dbus.\n";
        return 1;
    }

    runSync(KWIN_WAYLAND_BIN_PATH, { "--xwayland", "--libinput", "--exit-with-session=" CMAKE_INSTALL_FULL_LIBEXECDIR "/startplasma-waylandsession" });

    out << "startplasmacompositor: Shutting down...\n";
    cleanupPlasmaEnvironment();
    out << "startplasmacompositor: Done.\n";

    return 0;
}
