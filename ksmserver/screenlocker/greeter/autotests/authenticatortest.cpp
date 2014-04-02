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
#include "../authenticator.h"
// Qt
#include <QtTest/QtTest>

class AuthenticatorTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testIncorrectPassword();
    void testCorrectPassword();
    void testMessage();
    void testError();
};

void AuthenticatorTest::testIncorrectPassword()
{
    QScopedPointer<Authenticator> authenticator(new Authenticator(this));
    QSignalSpy failedSpy(authenticator.data(), SIGNAL(failed()));
    QSignalSpy succeededSpy(authenticator.data(), SIGNAL(succeeded()));
    QSignalSpy graceLockSpy(authenticator.data(), SIGNAL(graceLockedChanged()));
    QCOMPARE(authenticator->isGraceLocked(), false);
    authenticator->tryUnlock(QStringLiteral("incorrect"));
    QVERIFY(failedSpy.wait());
    QCOMPARE(authenticator->isGraceLocked(), true);
    QCOMPARE(failedSpy.count(), 1);
    QVERIFY(succeededSpy.isEmpty());
    QCOMPARE(graceLockSpy.count(), 1);
    graceLockSpy.clear();

    // we are currently grace locked, so correct password will fail
    authenticator->tryUnlock(QStringLiteral("testpassword"));
    QVERIFY(!succeededSpy.wait(100));
    QCOMPARE(failedSpy.count(), 2);

    QVERIFY(graceLockSpy.wait());
}

void AuthenticatorTest::testCorrectPassword()
{
    QScopedPointer<Authenticator> authenticator(new Authenticator(this));
    QSignalSpy succeededSpy(authenticator.data(), SIGNAL(succeeded()));
    QSignalSpy failedSpy(authenticator.data(), SIGNAL(failed()));
    authenticator->tryUnlock(QStringLiteral("testpassword"));
    QVERIFY(succeededSpy.wait());
    QCOMPARE(succeededSpy.count(), 1);
    QVERIFY(failedSpy.isEmpty());
}

void AuthenticatorTest::testMessage()
{
    QScopedPointer<Authenticator> authenticator(new Authenticator(this));
    QSignalSpy succeededSpy(authenticator.data(), SIGNAL(succeeded()));
    QSignalSpy messageSpy(authenticator.data(), SIGNAL(message(QString)));
    authenticator->tryUnlock(QStringLiteral("info"));
    QVERIFY(succeededSpy.wait());
    QCOMPARE(succeededSpy.count(), 1);

    QCOMPARE(messageSpy.count(), 1);
    QCOMPARE(messageSpy.first().at(0).toString(), QStringLiteral("You wanted some info, here you have it"));
}

void AuthenticatorTest::testError()
{
    QScopedPointer<Authenticator> authenticator(new Authenticator(this));
    QSignalSpy failedSpy(authenticator.data(), SIGNAL(failed()));
    QSignalSpy errorSpy(authenticator.data(), SIGNAL(error(QString)));
    authenticator->tryUnlock(QStringLiteral("error"));
    QVERIFY(failedSpy.wait());
    QCOMPARE(failedSpy.count(), 1);

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.first().at(0).toString(), QStringLiteral("The world is going to explode"));
}

QTEST_MAIN(AuthenticatorTest)
#include "authenticatortest.moc"
