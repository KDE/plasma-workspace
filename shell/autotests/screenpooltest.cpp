/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <QObject>

#include <QApplication>
#include <QDir>
#include <QScreen>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include "../screenpool.h"
#include "mockcompositor.h"
#include "xdgoutputv1.h"

#include "config-KDECI_BUILD.h"

using namespace MockCompositor;

class ScreenPoolTest : public QObject, DefaultCompositor
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testScreenInsertion();
    void testRedundantScreenInsertion();
    void testMoveOutOfRedundant();
    void testMoveInRedundant();
    void testOrderSwap();
    void testPrimarySwapToRedundant();
    void testMoveRedundantToMakePrimary();
    void testMoveInRedundantToLosePrimary();
    void testSecondScreenRemoval();
    void testThirdScreenRemoval();
    void testLastScreenRemoval();
    void testFakeToRealScreen();

private:
    ScreenPool *m_screenPool;
};

void ScreenPoolTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    qRegisterMetaType<QScreen *>();

    m_screenPool = new ScreenPool(this);

    QTRY_COMPARE(QGuiApplication::screens().size(), 1);
    QTRY_COMPARE(m_screenPool->screenOrder().size(), 1);
    QCOMPARE(QGuiApplication::screens().first()->name(), QStringLiteral("WL-1"));
    QCOMPARE(QGuiApplication::primaryScreen(), QGuiApplication::screens().first());
    QCOMPARE(QGuiApplication::primaryScreen(), m_screenPool->primaryScreen());
    QCOMPARE(m_screenPool->idForScreen(m_screenPool->primaryScreen()), 0);
    QCOMPARE(m_screenPool->screenForId(0)->name(), QStringLiteral("WL-1"));
}

void ScreenPoolTest::cleanupTestCase()
{
#if !KDECI_BUILD
    QCOMPOSITOR_COMPARE(getAll<Output>().size(), 1); // Only the default output should be left
    QTRY_COMPARE(QGuiApplication::screens().size(), 1);
    QTRY_VERIFY2(isClean(), qPrintable(dirtyMessage()));
#endif

    KConfigGroup cg(KSharedConfig::openConfig(), QStringLiteral("ScreenConnectors"));
    cg.deleteGroup();
    cg.sync();
}

void ScreenPoolTest::testScreenInsertion()
{
    QSignalSpy addedSpy(qGuiApp, SIGNAL(screenAdded(QScreen *)));
    QSignalSpy orderChangeSpy(m_screenPool, &ScreenPool::screenOrderChanged);

    // Add a new output
    exec([=] {
        OutputData data;
        data.mode.resolution = {1920, 1080};
        data.position = {1920, 0};
        data.physicalSize = data.mode.physicalSizeForDpi(96);
        // NOTE: assumes that when a screen is added it will already have the final geometry
        add<Output>(data);
        outputOrder()->setList({"WL-1", "WL-2"});
    });

    addedSpy.wait();

    orderChangeSpy.wait();

    QCOMPARE(orderChangeSpy.size(), 1);
    QCOMPARE(QGuiApplication::screens().size(), 2);
    QCOMPARE(m_screenPool->screenOrder().size(), 2);
    QCOMPARE(addedSpy.size(), 1);

    QScreen *newScreen = addedSpy.takeFirst().at(0).value<QScreen *>();
    QCOMPARE(newScreen->name(), QStringLiteral("WL-2"));
    QCOMPARE(newScreen->geometry(), QRect(1920, 0, 1920, 1080));
    // Check mapping
    QCOMPARE(m_screenPool->idForScreen(newScreen), 1);
    QCOMPARE(m_screenPool->screenForId(1)->name(), QStringLiteral("WL-2"));
}

void ScreenPoolTest::testRedundantScreenInsertion()
{
    QSignalSpy orderChangeSpy(m_screenPool, &ScreenPool::screenOrderChanged);
    QSignalSpy addedFromAppSpy(qGuiApp, SIGNAL(screenAdded(QScreen *)));

    // Add a new output
    exec([=] {
        OutputData data;
        data.mode.resolution = {1280, 720};
        data.position = {1920, 0};
        data.physicalSize = data.mode.physicalSizeForDpi(96);
        // NOTE: assumes that when a screen is added it will already have the final geometry
        add<Output>(data);
        outputOrder()->setList({"WL-1", "WL-2", "WL-3"});
    });

    addedFromAppSpy.wait();
    orderChangeSpy.wait(250);
    // only addedFromAppSpy will have registered something, nothing in orderChangeSpy,
    // on ScreenPool API POV is like this new screen doesn't exist, because is redundant to WL-2
    QCOMPARE(QGuiApplication::screens().size(), 3);
    QCOMPARE(m_screenPool->screenOrder().size(), 2);
    QCOMPARE(addedFromAppSpy.size(), 1);
    QCOMPARE(orderChangeSpy.size(), 0);

    QScreen *newScreen = addedFromAppSpy.takeFirst().at(0).value<QScreen *>();
    QCOMPARE(newScreen->name(), QStringLiteral("WL-3"));
    QCOMPARE(newScreen->geometry(), QRect(1920, 0, 1280, 720));
    QVERIFY(!m_screenPool->screenOrder().contains(newScreen));

    QCOMPARE(m_screenPool->idForScreen(newScreen), -1);
}

void ScreenPoolTest::testMoveOutOfRedundant()
{
    QSignalSpy orderChangeSpy(m_screenPool, &ScreenPool::screenOrderChanged);

    exec([=] {
        auto *out = output(2);
        auto *xdgOut = xdgOutput(out);
        out->m_data.mode.resolution = {1280, 2048};
        xdgOut->sendLogicalSize(QSize(1280, 2048));
        out->sendDone();
        outputOrder()->setList({"WL-1", "WL-2", "WL-3"});
    });

    orderChangeSpy.wait();
    QCOMPARE(orderChangeSpy.size(), 1);

    QList<QScreen *> newOrder = orderChangeSpy[0].at(0).value<QList<QScreen *>>();
    QCOMPARE(newOrder.count(), 3);
    QCOMPARE(newOrder[1]->name(), QStringLiteral("WL-2"));
    QCOMPARE(newOrder[1]->geometry(), QRect(1920, 0, 1920, 1080));
    QVERIFY(m_screenPool->screenOrder().contains(newOrder[0]));
}

void ScreenPoolTest::testMoveInRedundant()
{
    QSignalSpy removedSpy(m_screenPool, SIGNAL(screenRemoved(QScreen *)));

    exec([=] {
        auto *out = output(2);
        auto *xdgOut = xdgOutput(out);
        out->m_data.mode.resolution = {1280, 720};
        xdgOut->sendLogicalSize(QSize(1280, 720));
        out->sendDone();
        outputOrder()->setList({"WL-1", "WL-2", "WL-3"});
    });

    removedSpy.wait();
    QCOMPARE(removedSpy.size(), 1);
    QScreen *oldScreen = removedSpy.takeFirst().at(0).value<QScreen *>();
    QCOMPARE(oldScreen->name(), QStringLiteral("WL-3"));
    QCOMPARE(oldScreen->geometry(), QRect(1920, 0, 1280, 720));
    QVERIFY(!m_screenPool->screenOrder().contains(oldScreen));
}

void ScreenPoolTest::testOrderSwap()
{
    QSignalSpy orderChangeSpy(m_screenPool, &ScreenPool::screenOrderChanged);

    // Check ScreenPool mapping before switch
    QList<QScreen *> oldOrder = m_screenPool->screenOrder();
    QCOMPARE(oldOrder.count(), 2);
    QCOMPARE(oldOrder[0]->name(), QStringLiteral("WL-1"));
    QCOMPARE(oldOrder[1]->name(), QStringLiteral("WL-2"));

    QCOMPARE(m_screenPool->screenOrder()[0]->name(), QStringLiteral("WL-1"));
    QCOMPARE(m_screenPool->screenOrder()[1]->name(), QStringLiteral("WL-2"));

    // Set a primary screen
    // TODO: "WL-2", "WL-1", "WL-3" when tests are self contained
    exec([=] {
        outputOrder()->setList({"WL-2", "WL-1", "WL-3"});
    });

    orderChangeSpy.wait();

    QCOMPARE(orderChangeSpy.size(), 1);
    QList<QScreen *> newOrder = orderChangeSpy[0].at(0).value<QList<QScreen *>>();
    QCOMPARE(newOrder.count(), 2);
    QCOMPARE(newOrder[0], oldOrder[1]);
    QCOMPARE(newOrder[1], oldOrder[0]);

    QCOMPARE(newOrder[1]->name(), QStringLiteral("WL-1"));
    QCOMPARE(newOrder[1]->geometry(), QRect(0, 0, 1920, 1080));
    QCOMPARE(newOrder[0]->name(), QStringLiteral("WL-2"));
    QCOMPARE(newOrder[0]->geometry(), QRect(1920, 0, 1920, 1080));

    // Check ScreenPool mapping
    QCOMPARE(newOrder[0]->name(), QStringLiteral("WL-2"));
    QCOMPARE(m_screenPool->primaryScreen()->name(), newOrder[0]->name());
    QCOMPARE(m_screenPool->idForScreen(newOrder[0]), 0);
    QCOMPARE(m_screenPool->idForScreen(newOrder[1]), 1);
}

void ScreenPoolTest::testPrimarySwapToRedundant()
{
    QSignalSpy orderChangeSpy(m_screenPool, &ScreenPool::screenOrderChanged);

    // Set a primary screen
    exec([=] {
        outputOrder()->setList({"WL-3", "WL-2", "WL-1"});
    });

    orderChangeSpy.wait(250);
    // Nothing will happen, is like WL3 doesn't exist
    QCOMPARE(orderChangeSpy.size(), 0);
}

void ScreenPoolTest::testMoveRedundantToMakePrimary()
{
    QSignalSpy orderChangeSpy(m_screenPool, &ScreenPool::screenOrderChanged);

    exec([=] {
        auto *out = output(2);
        auto *xdgOut = xdgOutput(out);
        out->m_data.mode.resolution = {1280, 2048};
        xdgOut->sendLogicalSize(QSize(1280, 2048));
        out->sendDone();
        outputOrder()->setList({"WL-3", "WL-2", "WL-1"});
    });

    QTRY_COMPARE(orderChangeSpy.size(), 1);
    /*QScreen *newScreen = addedSpy.takeFirst().at(0).value<QScreen *>();
    QCOMPARE(newScreen->name(), QStringLiteral("WL-3"));
    QCOMPARE(newScreen->geometry(), QRect(1920, 0, 1280, 2048));
    QVERIFY(m_screenPool->screens().contains(newScreen));*/

    // Test the new primary
    QList<QScreen *> newOrder = orderChangeSpy[0].at(0).value<QList<QScreen *>>();
    QCOMPARE(newOrder.size(), 3);
    QCOMPARE(newOrder[0]->name(), QStringLiteral("WL-3"));
    QCOMPARE(newOrder[0]->geometry(), QRect(1920, 0, 1280, 2048));
    QCOMPARE(newOrder[1]->name(), QStringLiteral("WL-2"));
    QCOMPARE(newOrder[1]->geometry(), QRect(1920, 0, 1920, 1080));
    QCOMPARE(newOrder[2]->name(), QStringLiteral("WL-1"));
    QCOMPARE(newOrder[2]->geometry(), QRect(0, 0, 1920, 1080));

    // Check ScreenPool mapping
    QCOMPARE(m_screenPool->primaryScreen()->name(), QStringLiteral("WL-3"));
    QCOMPARE(m_screenPool->idForScreen(newOrder[0]), 0);
    QCOMPARE(m_screenPool->idForScreen(newOrder[1]), 1);
}

void ScreenPoolTest::testMoveInRedundantToLosePrimary()
{
    QSignalSpy orderChangeSpy(m_screenPool, &ScreenPool::screenOrderChanged);

    QCOMPARE(m_screenPool->screenOrder().size(), 3);
    QScreen *oldScreen = m_screenPool->screenOrder()[0];

    exec([=] {
        auto *out = output(2);
        auto *xdgOut = xdgOutput(out);
        xdgOut->sendLogicalSize(QSize(1280, 720));
        out->m_data.mode.resolution = {1280, 720};
        out->sendDone();
        outputOrder()->setList({"WL-3", "WL-2", "WL-1"});
    });

    QTRY_COMPARE(orderChangeSpy.size(), 1);
    QList<QScreen *> newOrder = orderChangeSpy[0].at(0).value<QList<QScreen *>>();
    QCOMPARE(newOrder.size(), 2);

    QCOMPARE(newOrder[1]->name(), QStringLiteral("WL-1"));
    QCOMPARE(newOrder[1]->geometry(), QRect(0, 0, 1920, 1080));
    QCOMPARE(newOrder[0]->name(), QStringLiteral("WL-2"));
    QCOMPARE(newOrder[0]->geometry(), QRect(1920, 0, 1920, 1080));

    // Check ScreenPool mapping
    QCOMPARE(newOrder[0]->name(), QStringLiteral("WL-2"));
    QCOMPARE(m_screenPool->primaryScreen()->name(), newOrder[0]->name());
    QCOMPARE(m_screenPool->idForScreen(newOrder[0]), 0);
    QCOMPARE(m_screenPool->idForScreen(newOrder[1]), 1);

    QVERIFY(!newOrder.contains(oldScreen));
    QCOMPARE(oldScreen->name(), QStringLiteral("WL-3"));
    QCOMPARE(oldScreen->geometry(), QRect(1920, 0, 1280, 720));
    QVERIFY(!m_screenPool->screenOrder().contains(oldScreen));
}

void ScreenPoolTest::testSecondScreenRemoval()
{
    QSignalSpy orderChangeSpy(m_screenPool, &ScreenPool::screenOrderChanged);
    QSignalSpy removedSpy(m_screenPool, SIGNAL(screenRemoved(QScreen *)));

    // Check ScreenPool mapping before switch
    QCOMPARE(m_screenPool->screenOrder()[0]->name(), QStringLiteral("WL-2"));
    QCOMPARE(m_screenPool->screenOrder()[1]->name(), QStringLiteral("WL-1"));

    // Remove an output
    exec([=] {
        remove(output(1));
        outputOrder()->setList({"WL-3", "WL-1"});
    });

    // Removing the primary screen, will change a primaryChange signal beforehand
    QTRY_COMPARE(orderChangeSpy.size(), 1);
    QCOMPARE(orderChangeSpy.size(), 1);
    QList<QScreen *> newOrder = orderChangeSpy[0].at(0).value<QList<QScreen *>>();
    QCOMPARE(newOrder.size(), 2);
    QCOMPARE(newOrder[0]->name(), QStringLiteral("WL-3"));
    QCOMPARE(newOrder[0]->geometry(), QRect(1920, 0, 1280, 720));
    QCOMPARE(newOrder[1]->name(), QStringLiteral("WL-1"));
    QCOMPARE(newOrder[1]->geometry(), QRect(0, 0, 1920, 1080));
}

void ScreenPoolTest::testThirdScreenRemoval()
{
    QSignalSpy removedSpy(m_screenPool, SIGNAL(screenRemoved(QScreen *)));
    QSignalSpy orderChangeSpy(m_screenPool, &ScreenPool::screenOrderChanged);

    // Remove an output
    exec([=] {
        remove(output(1));
        outputOrder()->setList({"WL-1"});
    });

    // NOTE: we can neither access the data of removedSpy nor oldPrimary because at this point will be dangling
    removedSpy.wait();
    QTRY_COMPARE(orderChangeSpy.size(), 1);

    QCOMPARE(orderChangeSpy.size(), 1);
    QList<QScreen *> newOrder = orderChangeSpy[0].at(0).value<QList<QScreen *>>();
    QCOMPARE(newOrder.size(), 1);
    QCOMPARE(QGuiApplication::screens().size(), 1);
    QCOMPARE(m_screenPool->screenOrder().size(), 1);
    QScreen *lastScreen = m_screenPool->screenOrder().first();
    QCOMPARE(lastScreen->name(), QStringLiteral("WL-1"));
    QCOMPARE(lastScreen->geometry(), QRect(0, 0, 1920, 1080));
    QCOMPARE(m_screenPool->screenOrder().first(), lastScreen);
    // This shouldn't have changed after removing a non primary screen
    QCOMPARE(m_screenPool->screenOrder()[0]->name(), QStringLiteral("WL-1"));
}

void ScreenPoolTest::testLastScreenRemoval()
{
    QSignalSpy removedSpy(m_screenPool, SIGNAL(screenRemoved(QScreen *)));

    // Remove an output
    exec([=] {
        remove(output(0));
        outputOrder()->setList({});
    });

    // NOTE: we can neither access the data of removedSpy nor oldPrimary because at this point will be dangling
    removedSpy.wait();

    QCOMPARE(QGuiApplication::screens().size(), 1);
    QCOMPARE(m_screenPool->screenOrder().size(), 0);
    QScreen *fakeScreen = QGuiApplication::screens().first();
    QCOMPARE(fakeScreen->name(), QString());
    QCOMPARE(fakeScreen->geometry(), QRect(0, 0, 0, 0));
}

void ScreenPoolTest::testFakeToRealScreen()
{
#if KDECI_BUILD
    QSKIP("testFakeToRealScreen causes assertion error in screenpool.cpp line 320");
#endif

    QSignalSpy orderChangeSpy(m_screenPool, &ScreenPool::screenOrderChanged);

    // Add a new output
    exec([=] {
        OutputData data;
        data.mode.resolution = {1920, 1080};
        data.position = {0, 0};
        data.physicalSize = data.mode.physicalSizeForDpi(96);
        auto *out = add<Output>(data);
        auto *xdgOut = xdgOutput(out);
        xdgOut->m_name = QStringLiteral("WL-1");
        outputOrder()->setList({"WL-1"});
    });

    orderChangeSpy.wait();
    QList<QScreen *> newOrder = orderChangeSpy[0].at(0).value<QList<QScreen *>>();
    QCOMPARE(newOrder.size(), 1);
    QCOMPARE(QGuiApplication::screens().size(), 1);
    QCOMPARE(m_screenPool->screenOrder().size(), 1);

    QScreen *newScreen = newOrder[0];
    QCOMPARE(newScreen, QGuiApplication::screens()[0]);
    QCOMPARE(newScreen->name(), QStringLiteral("WL-1"));
    QCOMPARE(newScreen->geometry(), QRect(0, 0, 1920, 1080));

    QCOMPARE(m_screenPool->idForScreen(newScreen), 0);
}

QCOMPOSITOR_TEST_MAIN(ScreenPoolTest)

#include "screenpooltest.moc"
