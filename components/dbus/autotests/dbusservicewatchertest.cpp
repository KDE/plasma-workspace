/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include <QProcess>
#include <QQmlComponent>
#include <QQmlProperty>
#include <QQuickItem>
#include <QQuickView>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;

class DBusServiceWatcherTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();

    void init();
    void cleanup();

    void testBusType();
    void testWatchedService();

private:
    QQuickView view;
    QObject *watcher = nullptr;
    QProcess dbus;
};

void DBusServiceWatcherTest::initTestCase()
{
    view.setSource(QUrl::fromLocalFile(QFINDTESTDATA(u"dbusservicewatchertest.qml"_s)));
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    watcher = view.rootObject()->property("watcher").value<QObject *>();
    QVERIFY(watcher);

    dbus.setProgram(u"python3"_s);
    dbus.setArguments({QFINDTESTDATA(u"dbusservice.py"_s)});
}

void DBusServiceWatcherTest::init()
{
    QVERIFY(!get<bool>(watcher->property("registered")));
    QSignalSpy spy(watcher, SIGNAL(registeredChanged()));
    dbus.start();
    QVERIFY(!spy.empty() || spy.wait());
    QVERIFY(get<bool>(watcher->property("registered")));
}

void DBusServiceWatcherTest::cleanup()
{
    QSignalSpy spy(watcher, SIGNAL(registeredChanged()));
    dbus.terminate();
    dbus.waitForFinished();
    QVERIFY(!spy.empty() || spy.wait());
    QVERIFY(!get<bool>(watcher->property("registered")));
}

void DBusServiceWatcherTest::testBusType()
{
    QSignalSpy spy(watcher, SIGNAL(registeredChanged()));
    QCOMPARE(watcher->property("busType").toInt(), 0);
    watcher->setProperty("busType", 1 /*System*/);
    QVERIFY(!spy.empty() || spy.wait());
    QVERIFY(!get<bool>(watcher->property("registered")));
    spy.clear();
    watcher->setProperty("busType", 0 /*Session*/);
    QVERIFY(!spy.empty() || spy.wait());
    QVERIFY(get<bool>(watcher->property("registered")));
}

void DBusServiceWatcherTest::testWatchedService()
{
    QSignalSpy spy(watcher, SIGNAL(registeredChanged()));
    QCOMPARE(get<QString>(watcher->property("watchedService")), u"org.kde.KSplash"_s);
    watcher->setProperty("watchedService", u"org.kde.invalid"_s);
    QVERIFY(!spy.empty() || spy.wait());
    QVERIFY(!get<bool>(watcher->property("registered")));

    spy.clear();
    watcher->setProperty("watchedService", u"org.kde.KSplash"_s);
    QVERIFY(!spy.empty() || spy.wait());
    QVERIFY(get<bool>(watcher->property("registered")));

    spy.clear();
    watcher->setProperty("watchedService", QString());
    QVERIFY(!spy.empty() || spy.wait());
    QVERIFY(!get<bool>(watcher->property("registered")));

    spy.clear();
    watcher->setProperty("watchedService", u"org.kde.KSplash"_s);
    QVERIFY(!spy.empty() || spy.wait());
    QVERIFY(get<bool>(watcher->property("registered")));

    watcher->setProperty("watchedService", u"org.kde.KSplash"_s);
    QVERIFY(get<bool>(watcher->property("registered")));
}

QTEST_MAIN(DBusServiceWatcherTest)

#include "dbusservicewatchertest.moc"
