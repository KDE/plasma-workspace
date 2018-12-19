/*
 * Copyright 2018  Martin Flöser <mgraesslin@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QtTest>

#include "../kworkspace.h"

#include <vector>

class TestPlatformDetection : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();
    void testPlatformSelection_data();
    void testPlatformSelection();
    void testArguments_data();
    void testArguments();
    void testQtQpaPlatformIsSet_data();
    void testQtQpaPlatformIsSet();
};

void TestPlatformDetection::init()
{
    qunsetenv("QT_QPA_PLATFORM");
    qunsetenv("XDG_SESSION_TYPE");
}

void TestPlatformDetection::testPlatformSelection_data()
{
    QTest::addColumn<QByteArray>("xdgSessionType");
    QTest::addColumn<QByteArray>("expectedQtQpaPlatform");

    QTest::newRow("wayland") << QByteArrayLiteral("wayland") << QByteArrayLiteral("wayland");
    QTest::newRow("x11") << QByteArrayLiteral("x11") << QByteArrayLiteral("xcb");
    QTest::newRow("unknown") << QByteArrayLiteral("mir") << QByteArray();
}

void TestPlatformDetection::testPlatformSelection()
{
    QVERIFY(!qEnvironmentVariableIsSet("QT_QPA_PLATFORM"));
    QFETCH(QByteArray, xdgSessionType);
    qputenv("XDG_SESSION_TYPE", xdgSessionType);

    std::vector<QByteArray> cppArgv{
        QByteArrayLiteral("testPlatformDetction")
    };
    std::vector<char*> argv;
    for (QByteArray &arg : cppArgv) {
        argv.push_back(arg.data());
    }
    KWorkSpace::detectPlatform(1, argv.data());
    QTEST(qgetenv("QT_QPA_PLATFORM"), "expectedQtQpaPlatform");
}

void TestPlatformDetection::testArguments_data()
{
    QTest::addColumn<QByteArray>("arg");

    QTest::newRow("-platform") << QByteArrayLiteral("-platform");
    QTest::newRow("--platform") << QByteArrayLiteral("--platform");
    QTest::newRow("-platform=wayland") << QByteArrayLiteral("-platform=wayland");
    QTest::newRow("--platform=wayland") << QByteArrayLiteral("--platform=wayland");
}

void TestPlatformDetection::testArguments()
{
    QVERIFY(!qEnvironmentVariableIsSet("QT_QPA_PLATFORM"));
    qputenv("XDG_SESSION_TYPE", "wayland");

    QFETCH(QByteArray, arg);
    std::vector<QByteArray> cppArgv{
        QByteArrayLiteral("testPlatformDetction"),
        arg,
        QByteArrayLiteral("wayland")
    };
    std::vector<char*> argv;
    for (QByteArray &arg : cppArgv) {
        argv.push_back(arg.data());
    }
    KWorkSpace::detectPlatform(3, argv.data());
    QVERIFY(!qEnvironmentVariableIsSet("QT_QPA_PLATFORM"));
}

void TestPlatformDetection::testQtQpaPlatformIsSet_data()
{
    QTest::addColumn<QByteArray>("qtQpaPlatform");
    QTest::addColumn<QByteArray>("xdgSessionType");

    QTest::newRow("xcb - x11") << QByteArrayLiteral("xcb") << QByteArrayLiteral("x11");
    QTest::newRow("xcb - wayland") << QByteArrayLiteral("xcb") << QByteArrayLiteral("wayland");
    QTest::newRow("wayland - x11") << QByteArrayLiteral("wayland") << QByteArrayLiteral("x11");
    QTest::newRow("wayland - wayland") << QByteArrayLiteral("wayland") << QByteArrayLiteral("wayland");
    QTest::newRow("windows - x11") << QByteArrayLiteral("windows") << QByteArrayLiteral("x11");
}

void TestPlatformDetection::testQtQpaPlatformIsSet()
{
    // test verifies that if QT_QPA_PLATFORM is set the env variable does not get adjusted
    QFETCH(QByteArray, qtQpaPlatform);
    QFETCH(QByteArray, xdgSessionType);
    qputenv("QT_QPA_PLATFORM", qtQpaPlatform);
    qputenv("XDG_SESSION_TYPE", xdgSessionType);

    KWorkSpace::detectPlatform(0, nullptr);
    QCOMPARE(qgetenv("QT_QPA_PLATFORM"), qtQpaPlatform);
}

QTEST_GUILESS_MAIN(TestPlatformDetection)

#include "testPlatformDetection.moc"
