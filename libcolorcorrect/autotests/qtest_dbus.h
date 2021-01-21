/*
    Copyright 2014 Alejandro Fiestas Olivares <afiestas@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef COLORCORRECT_AUTOTESTS_QTEST_DBUS_H
#define COLORCORRECT_AUTOTESTS_QTEST_DBUS_H

#include <QtTest>
#include <stdlib.h>

#define QTEST_GUILESS_MAIN_SYSTEM_DBUS(TestObject)                                                                                                             \
    int main(int argc, char *argv[])                                                                                                                           \
    {                                                                                                                                                          \
        QProcess dbus;                                                                                                                                         \
        dbus.start(QStringLiteral("dbus-launch"), QStringList());                                                                                              \
        dbus.waitForFinished(10000);                                                                                                                           \
        QByteArray session = dbus.readLine();                                                                                                                  \
        if (session.isEmpty()) {                                                                                                                               \
            qFatal("Couldn't execute new dbus session");                                                                                                       \
        }                                                                                                                                                      \
        int pos = session.indexOf('=');                                                                                                                        \
        qputenv("DBUS_SESSION_BUS_ADDRESS", session.right(session.count() - pos - 1).trimmed().constData());                                                   \
        session = dbus.readLine();                                                                                                                             \
        pos = session.indexOf('=');                                                                                                                            \
        QByteArray pid = session.right(session.count() - pos - 1).trimmed();                                                                                   \
        QCoreApplication app(argc, argv);                                                                                                                      \
        app.setApplicationName(QLatin1String("qttest"));                                                                                                       \
        TestObject tc;                                                                                                                                         \
        int result = QTest::qExec(&tc, argc, argv);                                                                                                            \
        dbus.start(QStringLiteral("kill"), QStringList() << QStringLiteral("-9") << QString::fromLatin1(pid));                                                 \
        dbus.waitForFinished();                                                                                                                                \
        return result;                                                                                                                                         \
    }
#endif // COLORCORRECT_AUTOTESTS_QTEST_DBUS_H
