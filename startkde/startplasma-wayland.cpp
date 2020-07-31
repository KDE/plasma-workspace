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
#include <QDBusConnection>
#include <QDBusInterface>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    createConfigDirectory();
    setupCursor(true);

    {
        KConfig fonts(QStringLiteral("kcmfonts"));
        KConfigGroup group = fonts.group("General");
        auto dpiSetting = group.readEntry("forceFontDPIWayland", 96);
        auto dpi = dpiSetting == 0 ? 96 : dpiSetting;
        qputenv("QT_WAYLAND_FORCE_DPI", QByteArray::number(dpi));
    }

    // Query whether org.freedesktop.locale1 is available. If it is, try to
    // set XKB_DEFAULT_{MODEL,LAYOUT,VARIANT,OPTIONS} accordingly.
    {
        const QString locale1Service = QStringLiteral("org.freedesktop.locale1");
        const QString locale1Path = QStringLiteral("/org/freedesktop/locale1");
        QDBusMessage message = QDBusMessage::createMethodCall(locale1Service,
                                                        locale1Path,
                                                        QStringLiteral("org.freedesktop.DBus.Properties"),
                                                        QLatin1String("GetAll"));
        message << locale1Service;
        QDBusMessage resultMessage = QDBusConnection::systemBus().call(message);
        if (resultMessage.type() == QDBusMessage::ReplyMessage) {
            QVariantMap result;
            QDBusArgument dbusArgument = resultMessage.arguments().at(0).value<QDBusArgument>();
            while (!dbusArgument.atEnd()) {
                dbusArgument >> result;
            }

            auto queryAndSet = [&](const QByteArray &var, const QString & value) {
                const auto r = result.value(value).toString();
                if (!r.isEmpty())
                    qputenv(var, r.toUtf8());
            };

            queryAndSet("X11MODEL", QStringLiteral("X11Model"));
            queryAndSet("X11LAYOUT", QStringLiteral("X11Layout"));
            queryAndSet("X11VARIANT", QStringLiteral("X11Variant"));
            queryAndSet("X11OPTIONS", QStringLiteral("X11Options"));
        } else {
            qWarning() << "not a reply org.freedesktop.locale1" << resultMessage;
        }
    }
    runEnvironmentScripts();

    if (!qEnvironmentVariableIsSet("DBUS_SESSION_BUS_ADDRESS")) {
        out << "startplasmacompositor: Could not start D-Bus. Can you call qdbus?\n";
        return 1;
    }
    setupPlasmaEnvironment();
    runStartupConfig();
    qputenv("PLASMA_USE_QT_SCALING", "1");

    qputenv("XDG_SESSION_TYPE", "wayland");

    if (!syncDBusEnvironment()) {
        out << "Could not sync environment to dbus.\n";
        return 1;
    }

    QStringList args;
    if (argc > 1) {
        args.reserve(argc);
        for (int i = 1; i < argc; ++i) {
            args << QString::fromLocal8Bit(argv[i]);
        }
    } else {
        args = QStringList { QStringLiteral("--xwayland"), QStringLiteral("--exit-with-session=" CMAKE_INSTALL_FULL_LIBEXECDIR "/startplasma-waylandsession") };
    }
    runSync(QStringLiteral(KWIN_WAYLAND_BIN_PATH), args);

    out << "startplasmacompositor: Shutting down...\n";
    cleanupPlasmaEnvironment();
    out << "startplasmacompositor: Done.\n";

    return 0;
}
