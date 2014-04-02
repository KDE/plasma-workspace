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
#include "../logind.h"
#include "fakelogind.h"
// Qt
#include <QtTest/QtTest>

class LogindTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testLockUnlock();
    void testLogindPresent();
    void testRegisterUnregister();
};

void LogindTest::testLockUnlock()
{
    QScopedPointer<LogindIntegration> logindIntegration(new LogindIntegration(QDBusConnection::sessionBus(), this));
    QSignalSpy lockSpy(logindIntegration.data(), SIGNAL(requestLock()));
    QSignalSpy unlockSpy(logindIntegration.data(), SIGNAL(requestUnlock()));
    QSignalSpy connectedSpy(logindIntegration.data(), SIGNAL(connectedChanged()));

    FakeLogind fakeLogind;

    // need to wait till we got the pending reply
    QVERIFY(connectedSpy.wait());
    QVERIFY(logindIntegration->isConnected());

    fakeLogind.lock();
    fakeLogind.lock();

    QVERIFY(lockSpy.wait());
    QCOMPARE(lockSpy.count(), 2);

    fakeLogind.unlock();
    QVERIFY(unlockSpy.wait());
    QCOMPARE(unlockSpy.count(), 1);
}

void LogindTest::testLogindPresent()
{
    QTest::qWait(100);
    FakeLogind fakeLogind;
    QScopedPointer<LogindIntegration> logindIntegration(new LogindIntegration(QDBusConnection::sessionBus(), this));

    QSignalSpy connectedSpy(logindIntegration.data(), SIGNAL(connectedChanged()));
    QVERIFY(connectedSpy.wait());
    QVERIFY(logindIntegration->isConnected());

    QSignalSpy lockSpy(logindIntegration.data(), SIGNAL(requestLock()));
    fakeLogind.lock();
    QVERIFY(lockSpy.wait());
    QCOMPARE(lockSpy.count(), 1);
}

void LogindTest::testRegisterUnregister()
{
    QTest::qWait(100);
    QScopedPointer<FakeLogind> fakeLogind(new FakeLogind(this));
    QScopedPointer<LogindIntegration> logindIntegration(new LogindIntegration(QDBusConnection::sessionBus(), this));

    // should get connected
    QSignalSpy connectedSpy(logindIntegration.data(), SIGNAL(connectedChanged()));
    QVERIFY(connectedSpy.wait());
    QVERIFY(logindIntegration->isConnected());
    connectedSpy.clear();

    fakeLogind.reset();
    // should no longer be connected
    QVERIFY(connectedSpy.wait());
    QVERIFY(!logindIntegration->isConnected());
    connectedSpy.clear();

    fakeLogind.reset(new FakeLogind(this));
    // should be connected again
    QVERIFY(connectedSpy.wait());
    QVERIFY(logindIntegration->isConnected());
}

QTEST_MAIN(LogindTest)
#include "logindtest.moc"
