/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QProcess>
#include <QQmlComponent>
#include <QQmlProperty>
#include <QQuickItem>
#include <QQuickView>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;

class KeyStateTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();

    // Caps Lock and Num Lock
    void testLock();

    // Shift
    void testLatch();

    // Ignore non-modifier keys
    void testUnknownKey();

private:
    QQuickView view;
    QObject *keyState = nullptr;
};

void KeyStateTest::initTestCase()
{
    view.setSource(QUrl::fromLocalFile(QFINDTESTDATA(u"keystatetest.qml"_s)));
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    keyState = view.rootObject()->property("keyState").value<QObject *>();
    QVERIFY(keyState);
}

void KeyStateTest::testLock()
{
    // Caps Lock
    QCOMPARE(get<Qt::Key>(keyState->property("key")), Qt::Key_CapsLock);
    QVERIFY(keyState->setProperty("key", static_cast<int>(Qt::Key_CapsLock)));
    QVERIFY(!get<bool>(keyState->property("locked")));

    QSignalSpy lockedSpy(keyState, SIGNAL(lockedChanged()));
    QProcess xdotool;
    xdotool.start(u"xdotool"_s, {u"key"_s, u"Caps_Lock"_s});
    xdotool.waitForFinished();
    QVERIFY(!lockedSpy.empty() || lockedSpy.wait());
    QVERIFY(get<bool>(keyState->property("locked")));

    lockedSpy.clear();
    xdotool.start(u"xdotool"_s, {u"key"_s, u"Caps_Lock"_s});
    xdotool.waitForFinished();
    QVERIFY(!lockedSpy.empty() || lockedSpy.wait());
    QVERIFY(!get<bool>(keyState->property("locked")));

    lockedSpy.clear();
    QMetaObject::invokeMethod(keyState, "lock", Q_ARG(bool, true));
    QVERIFY(!lockedSpy.empty() || lockedSpy.wait());
    QVERIFY(get<bool>(keyState->property("locked")));

    lockedSpy.clear();
    QMetaObject::invokeMethod(keyState, "lock", Q_ARG(bool, false));
    QVERIFY(!lockedSpy.empty() || lockedSpy.wait());
    QVERIFY(!get<bool>(keyState->property("locked")));

    // Num Lock
    QVERIFY(keyState->setProperty("key", static_cast<int>(Qt::Key_NumLock)));
    QVERIFY(!get<bool>(keyState->property("locked")));
    QCOMPARE(get<Qt::Key>(keyState->property("key")), Qt::Key_NumLock);

    lockedSpy.clear();
    xdotool.start(u"xdotool"_s, {u"key"_s, u"Num_Lock"_s});
    xdotool.waitForFinished();
    QVERIFY(!lockedSpy.empty() || lockedSpy.wait());
    QVERIFY(get<bool>(keyState->property("locked")));

    lockedSpy.clear();
    xdotool.start(u"xdotool"_s, {u"key"_s, u"Num_Lock"_s});
    xdotool.waitForFinished();
    QVERIFY(!lockedSpy.empty() || lockedSpy.wait());
    QVERIFY(!get<bool>(keyState->property("locked")));

    lockedSpy.clear();
    QMetaObject::invokeMethod(keyState, "lock", Q_ARG(bool, true));
    QVERIFY(!lockedSpy.empty() || lockedSpy.wait());
    QVERIFY(get<bool>(keyState->property("locked")));

    lockedSpy.clear();
    QMetaObject::invokeMethod(keyState, "lock", Q_ARG(bool, false));
    QVERIFY(!lockedSpy.empty() || lockedSpy.wait());
    QVERIFY(!get<bool>(keyState->property("locked")));
}

void KeyStateTest::testLatch()
{
    QVERIFY(keyState->setProperty("key", static_cast<int>(Qt::Key_Shift)));
    QVERIFY(!get<bool>(keyState->property("latched")));

    QSignalSpy lockedSpy(keyState, SIGNAL(latchedChanged()));
    QMetaObject::invokeMethod(keyState, "latch", Q_ARG(bool, true));
    QVERIFY(!lockedSpy.empty() || lockedSpy.wait());
    QVERIFY(get<bool>(keyState->property("latched")));

    lockedSpy.clear();
    QMetaObject::invokeMethod(keyState, "latch", Q_ARG(bool, false));
    QVERIFY(!lockedSpy.empty() || lockedSpy.wait());
    QVERIFY(!get<bool>(keyState->property("latched")));
}

void KeyStateTest::testUnknownKey()
{
    QVERIFY(keyState->setProperty("key", static_cast<int>(Qt::Key_CapsLock)));
    QSignalSpy lockedSpy(keyState, SIGNAL(lockedChanged()));
    QProcess xdotool;
    xdotool.start(u"xdotool"_s, {u"key"_s, u"Caps_Lock"_s});
    xdotool.waitForFinished();
    QVERIFY(!lockedSpy.empty() || lockedSpy.wait());
    QVERIFY(get<bool>(keyState->property("locked")));

    lockedSpy.clear();
    QVERIFY(keyState->setProperty("key", static_cast<int>(Qt::Key_A))); // Not a modifier
    QVERIFY(!lockedSpy.empty() || lockedSpy.wait());
    QCOMPARE(get<Qt::Key>(keyState->property("key")), Qt::Key_A);
    QVERIFY(!get<bool>(keyState->property("locked")));

    // Restore
    xdotool.start(u"xdotool"_s, {u"key"_s, u"Caps_Lock"_s});
    xdotool.waitForFinished();
}

QTEST_MAIN(KeyStateTest)

#include "keystatetest.moc"
