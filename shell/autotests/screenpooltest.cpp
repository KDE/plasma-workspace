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
    void testPrimarySwap();
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

    KConfigGroup cg(KSharedConfig::openConfig(), QStringLiteral("ScreenConnectors"));
    cg.deleteGroup();
    cg.sync();
    m_screenPool = new ScreenPool(KSharedConfig::openConfig(), this);
    m_screenPool->load();

    QTRY_COMPARE(QGuiApplication::screens().size(), 1);
    QCOMPARE(m_screenPool->screens().size(), 1);
    QCOMPARE(QGuiApplication::screens().first()->name(), QStringLiteral("WL-1"));
    QCOMPARE(QGuiApplication::primaryScreen(), QGuiApplication::screens().first());
    QCOMPARE(QGuiApplication::primaryScreen(), m_screenPool->primaryScreen());
    QCOMPARE(m_screenPool->id(m_screenPool->primaryScreen()->name()), 0);
    QCOMPARE(m_screenPool->connector(0), QStringLiteral("WL-1"));
}

void ScreenPoolTest::cleanupTestCase()
{
    QCOMPOSITOR_COMPARE(getAll<Output>().size(), 1); // Only the default output should be left
    QTRY_COMPARE(QGuiApplication::screens().size(), 1);
    QTRY_VERIFY2(isClean(), qPrintable(dirtyMessage()));

    KConfigGroup cg(KSharedConfig::openConfig(), QStringLiteral("ScreenConnectors"));
    cg.deleteGroup();
    cg.sync();
}

void ScreenPoolTest::testScreenInsertion()
{
    QSignalSpy addedSpy(m_screenPool, SIGNAL(screenAdded(QScreen *)));

    // Add a new output
    exec([=] {
        OutputData data;
        data.mode.resolution = {1920, 1080};
        data.position = {1920, 0};
        data.physicalSize = data.mode.physicalSizeForDpi(96);
        // NOTE: assumes that when a screen is added it will already have the final geometry
        add<Output>(data);
    });

    addedSpy.wait();
    QCOMPARE(QGuiApplication::screens().size(), 2);
    QCOMPARE(m_screenPool->screens().size(), 2);
    QCOMPARE(addedSpy.size(), 1);

    QScreen *newScreen = addedSpy.takeFirst().at(0).value<QScreen *>();
    QCOMPARE(newScreen->name(), QStringLiteral("WL-2"));
    QCOMPARE(newScreen->geometry(), QRect(1920, 0, 1920, 1080));
    // Check mapping
    QCOMPARE(m_screenPool->id(newScreen->name()), 1);
    QCOMPARE(m_screenPool->connector(1), QStringLiteral("WL-2"));
}

void ScreenPoolTest::testRedundantScreenInsertion()
{
    QSignalSpy addedSpy(m_screenPool, SIGNAL(screenAdded(QScreen *)));
    QSignalSpy addedFromAppSpy(qGuiApp, SIGNAL(screenAdded(QScreen *)));

    // Add a new output
    exec([=] {
        OutputData data;
        data.mode.resolution = {1280, 720};
        data.position = {1920, 0};
        data.physicalSize = data.mode.physicalSizeForDpi(96);
        // NOTE: assumes that when a screen is added it will already have the final geometry
        add<Output>(data);
    });

    addedFromAppSpy.wait();
    addedSpy.wait(250);
    // only addedFromAppSpy will have registered something, nothing in addedSpy,
    // on ScreenPool API POV is like this new screen doesn't exist, because is redundant to WL-2
    QCOMPARE(QGuiApplication::screens().size(), 3);
    QCOMPARE(m_screenPool->screens().size(), 2);
    QCOMPARE(addedFromAppSpy.size(), 1);
    QCOMPARE(addedSpy.size(), 0);

    QScreen *newScreen = addedFromAppSpy.takeFirst().at(0).value<QScreen *>();
    QCOMPARE(newScreen->name(), QStringLiteral("WL-3"));
    QCOMPARE(newScreen->geometry(), QRect(1920, 0, 1280, 720));
    QVERIFY(!m_screenPool->screens().contains(newScreen));

    QCOMPARE(m_screenPool->id(newScreen->name()), 2);
    QCOMPARE(m_screenPool->connector(2), QStringLiteral("WL-3"));
}

void ScreenPoolTest::testMoveOutOfRedundant()
{
    QSignalSpy addedSpy(m_screenPool, SIGNAL(screenAdded(QScreen *)));

    exec([=] {
        auto *out = output(2);
        auto *xdgOut = xdgOutput(out);
        out->m_data.mode.resolution = {1280, 2048};
        xdgOut->sendLogicalSize(QSize(1280, 2048));
        out->sendDone();
    });

    addedSpy.wait();
    QCOMPARE(addedSpy.size(), 1);
    QScreen *newScreen = addedSpy.takeFirst().at(0).value<QScreen *>();
    QCOMPARE(newScreen->name(), QStringLiteral("WL-3"));
    QCOMPARE(newScreen->geometry(), QRect(1920, 0, 1280, 2048));
    QVERIFY(m_screenPool->screens().contains(newScreen));
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
    });

    removedSpy.wait();
    QCOMPARE(removedSpy.size(), 1);
    QScreen *oldScreen = removedSpy.takeFirst().at(0).value<QScreen *>();
    QCOMPARE(oldScreen->name(), QStringLiteral("WL-3"));
    QCOMPARE(oldScreen->geometry(), QRect(1920, 0, 1280, 720));
    QVERIFY(!m_screenPool->screens().contains(oldScreen));
}

void ScreenPoolTest::testPrimarySwap()
{
    QSignalSpy primaryChangeSpy(m_screenPool, SIGNAL(primaryScreenChanged(QScreen *, QScreen *)));

    // Check ScreenPool mapping before switch
    QCOMPARE(m_screenPool->primaryConnector(), QStringLiteral("WL-1"));
    QCOMPARE(m_screenPool->primaryScreen()->name(), m_screenPool->primaryConnector());
    QCOMPARE(m_screenPool->id(QStringLiteral("WL-1")), 0);
    QCOMPARE(m_screenPool->id(QStringLiteral("WL-2")), 1);

    // Set a primary screen
    exec([=] {
        primaryOutput()->setPrimaryOutputName("WL-2");
    });

    primaryChangeSpy.wait();

    QCOMPARE(primaryChangeSpy.size(), 1);
    QScreen *oldPrimary = primaryChangeSpy[0].at(0).value<QScreen *>();
    QScreen *newPrimary = primaryChangeSpy[0].at(1).value<QScreen *>();
    QCOMPARE(oldPrimary->name(), QStringLiteral("WL-1"));
    QCOMPARE(oldPrimary->geometry(), QRect(0, 0, 1920, 1080));
    QCOMPARE(newPrimary->name(), QStringLiteral("WL-2"));
    QCOMPARE(newPrimary->geometry(), QRect(1920, 0, 1920, 1080));

    // Check ScreenPool mapping
    QCOMPARE(m_screenPool->primaryConnector(), QStringLiteral("WL-2"));
    QCOMPARE(m_screenPool->primaryConnector(), newPrimary->name());
    QCOMPARE(m_screenPool->primaryScreen()->name(), m_screenPool->primaryConnector());
    QCOMPARE(m_screenPool->id(newPrimary->name()), 0);
    QCOMPARE(m_screenPool->id(oldPrimary->name()), 1);
}

void ScreenPoolTest::testPrimarySwapToRedundant()
{
    QSignalSpy primaryChangeSpy(m_screenPool, SIGNAL(primaryScreenChanged(QScreen *, QScreen *)));

    // Set a primary screen
    exec([=] {
        primaryOutput()->setPrimaryOutputName("WL-3");
    });

    primaryChangeSpy.wait(250);
    // Nothing will happen, is like WL3 doesn't exist
    QCOMPARE(primaryChangeSpy.size(), 0);
}

void ScreenPoolTest::testMoveRedundantToMakePrimary()
{
    QSignalSpy addedSpy(m_screenPool, SIGNAL(screenAdded(QScreen *)));
    QSignalSpy primaryChangeSpy(m_screenPool, SIGNAL(primaryScreenChanged(QScreen *, QScreen *)));

    exec([=] {
        auto *out = output(2);
        auto *xdgOut = xdgOutput(out);
        out->m_data.mode.resolution = {1280, 2048};
        xdgOut->sendLogicalSize(QSize(1280, 2048));
        out->sendDone();
    });

    // Having multiple spies, when the wait of one will exit both signals will already have been emitted
    QTRY_COMPARE(addedSpy.size(), 1);
    QTRY_COMPARE(primaryChangeSpy.size(), 1);
    QScreen *newScreen = addedSpy.takeFirst().at(0).value<QScreen *>();
    QCOMPARE(newScreen->name(), QStringLiteral("WL-3"));
    QCOMPARE(newScreen->geometry(), QRect(1920, 0, 1280, 2048));
    QVERIFY(m_screenPool->screens().contains(newScreen));

    // Test the new primary
    QScreen *oldPrimary = primaryChangeSpy[0].at(0).value<QScreen *>();
    QScreen *newPrimary = primaryChangeSpy[0].at(1).value<QScreen *>();
    QCOMPARE(oldPrimary->name(), QStringLiteral("WL-2"));
    QCOMPARE(oldPrimary->geometry(), QRect(1920, 0, 1920, 1080));
    QCOMPARE(newPrimary->name(), QStringLiteral("WL-3"));
    QCOMPARE(newPrimary->geometry(), QRect(1920, 0, 1280, 2048));

    // Check ScreenPool mapping
    QCOMPARE(m_screenPool->primaryConnector(), QStringLiteral("WL-3"));
    QCOMPARE(m_screenPool->primaryConnector(), newPrimary->name());
    QCOMPARE(m_screenPool->primaryScreen()->name(), m_screenPool->primaryConnector());
    QCOMPARE(m_screenPool->id(newPrimary->name()), 0);
    QCOMPARE(m_screenPool->id(oldPrimary->name()), 2);
}

void ScreenPoolTest::testMoveInRedundantToLosePrimary()
{
    QSignalSpy removedSpy(m_screenPool, SIGNAL(screenRemoved(QScreen *)));
    QSignalSpy primaryChangeSpy(m_screenPool, SIGNAL(primaryScreenChanged(QScreen *, QScreen *)));

    exec([=] {
        auto *out = output(2);
        auto *xdgOut = xdgOutput(out);
        xdgOut->sendLogicalSize(QSize(1280, 720));
        out->m_data.mode.resolution = {1280, 720};
        out->sendDone();
    });

    QTRY_COMPARE(primaryChangeSpy.size(), 1);
    // Test the new primary
    QScreen *oldPrimary = primaryChangeSpy[0].at(0).value<QScreen *>();
    QScreen *newPrimary = primaryChangeSpy[0].at(1).value<QScreen *>();
    QCOMPARE(oldPrimary->name(), QStringLiteral("WL-3"));
    QCOMPARE(oldPrimary->geometry(), QRect(1920, 0, 1280, 720));
    QCOMPARE(newPrimary->name(), QStringLiteral("WL-2"));
    QCOMPARE(newPrimary->geometry(), QRect(1920, 0, 1920, 1080));

    // Check ScreenPool mapping
    QCOMPARE(m_screenPool->primaryConnector(), QStringLiteral("WL-2"));
    QCOMPARE(m_screenPool->primaryConnector(), newPrimary->name());
    QCOMPARE(m_screenPool->primaryScreen()->name(), m_screenPool->primaryConnector());
    QCOMPARE(m_screenPool->id(newPrimary->name()), 0);
    QCOMPARE(m_screenPool->id(oldPrimary->name()), 2);

    QTRY_COMPARE(removedSpy.size(), 1);
    QScreen *oldScreen = removedSpy.takeFirst().at(0).value<QScreen *>();
    QCOMPARE(oldScreen->name(), QStringLiteral("WL-3"));
    QCOMPARE(oldScreen->geometry(), QRect(1920, 0, 1280, 720));
    QVERIFY(!m_screenPool->screens().contains(oldScreen));
}

void ScreenPoolTest::testSecondScreenRemoval()
{
    QSignalSpy addedSpy(m_screenPool, SIGNAL(screenAdded(QScreen *)));
    QSignalSpy primaryChangeSpy(m_screenPool, SIGNAL(primaryScreenChanged(QScreen *, QScreen *)));
    QSignalSpy removedSpy(m_screenPool, SIGNAL(screenRemoved(QScreen *)));

    // Check ScreenPool mapping before switch
    QCOMPARE(m_screenPool->primaryConnector(), QStringLiteral("WL-2"));
    QCOMPARE(m_screenPool->primaryScreen()->name(), m_screenPool->primaryConnector());
    QCOMPARE(m_screenPool->id(QStringLiteral("WL-2")), 0);
    QCOMPARE(m_screenPool->id(QStringLiteral("WL-1")), 1);

    // Remove an output
    exec([=] {
        remove(output(1));
    });

    // Removing the primary screen, will change a primaryChange signal beforehand
    QTRY_COMPARE(primaryChangeSpy.size(), 1);
    QCOMPARE(primaryChangeSpy.size(), 1);
    QScreen *newPrimary = primaryChangeSpy[0].at(1).value<QScreen *>();
    QCOMPARE(newPrimary->name(), QStringLiteral("WL-3"));
    QCOMPARE(newPrimary->geometry(), QRect(1920, 0, 1280, 720));

    // Check ScreenPool mapping
    QCOMPARE(m_screenPool->primaryConnector(), QStringLiteral("WL-3"));
    QCOMPARE(m_screenPool->primaryScreen()->name(), m_screenPool->primaryConnector());
    QCOMPARE(m_screenPool->id(newPrimary->name()), 0);
    QCOMPARE(m_screenPool->id("WL-2"), 2);

    // NOTE: we can neither access the data of removedSpy nor oldPrimary because at this point will be dangling
    QTRY_COMPARE(removedSpy.size(), 1);
    QTRY_COMPARE(addedSpy.size(), 1);

    QCOMPARE(QGuiApplication::screens().size(), 2);
    QCOMPARE(m_screenPool->screens().size(), 2);
    QScreen *firstScreen = m_screenPool->screens().at(1);
    QCOMPARE(firstScreen, newPrimary);
    QCOMPARE(m_screenPool->primaryScreen(), newPrimary);

    // We'll get an added signal for the screen WL-3 that was previously redundant to WL-2
    QScreen *newScreen = addedSpy[0].at(0).value<QScreen *>();
    QCOMPARE(newScreen->name(), QStringLiteral("WL-3"));
    QCOMPARE(newScreen->geometry(), QRect(1920, 0, 1280, 720));
    QCOMPARE(m_screenPool->screens().at(1), newScreen);
}

void ScreenPoolTest::testThirdScreenRemoval()
{
    QSignalSpy removedSpy(m_screenPool, SIGNAL(screenRemoved(QScreen *)));

    // Remove an output
    exec([=] {
        // NOTE: Assume the server will always do the right thing to change the primary screen before deleting one
        primaryOutput()->setPrimaryOutputName("WL-1");

        remove(output(1));
    });

    // NOTE: we can neither access the data of removedSpy nor oldPrimary because at this point will be dangling
    removedSpy.wait();
    QCOMPARE(QGuiApplication::screens().size(), 1);
    QCOMPARE(m_screenPool->screens().size(), 1);
    QScreen *lastScreen = m_screenPool->screens().first();
    QCOMPARE(lastScreen->name(), QStringLiteral("WL-1"));
    QCOMPARE(lastScreen->geometry(), QRect(0, 0, 1920, 1080));
    QCOMPARE(m_screenPool->screens().first(), lastScreen);
    // This shouldn't have changed after removing a non primary screen
    QCOMPARE(m_screenPool->primaryConnector(), QStringLiteral("WL-1"));
}

void ScreenPoolTest::testLastScreenRemoval()
{
    QSignalSpy removedSpy(m_screenPool, SIGNAL(screenRemoved(QScreen *)));

    // Remove an output
    exec([=] {
        remove(output(0));
    });

    // NOTE: we can neither access the data of removedSpy nor oldPrimary because at this point will be dangling
    removedSpy.wait();
    QCOMPARE(QGuiApplication::screens().size(), 1);
    QCOMPARE(m_screenPool->screens().size(), 0);
    QScreen *fakeScreen = QGuiApplication::screens().first();
    QCOMPARE(fakeScreen->name(), QString());
    QCOMPARE(fakeScreen->geometry(), QRect(0, 0, 0, 0));
}

void ScreenPoolTest::testFakeToRealScreen()
{
    QSignalSpy addedSpy(m_screenPool, SIGNAL(screenAdded(QScreen *)));

    // Add a new output
    exec([=] {
        OutputData data;
        data.mode.resolution = {1920, 1080};
        data.position = {0, 0};
        data.physicalSize = data.mode.physicalSizeForDpi(96);
        auto *out = add<Output>(data);
        auto *xdgOut = xdgOutput(out);
        xdgOut->m_name = QStringLiteral("WL-1");
    });

    addedSpy.wait();
    QCOMPARE(QGuiApplication::screens().size(), 1);
    QCOMPARE(m_screenPool->screens().size(), 1);
    QCOMPARE(addedSpy.size(), 1);

    QScreen *newScreen = addedSpy.takeFirst().at(0).value<QScreen *>();
    QCOMPARE(newScreen->name(), QStringLiteral("WL-1"));
    QCOMPARE(newScreen->geometry(), QRect(0, 0, 1920, 1080));

    QCOMPARE(m_screenPool->id(newScreen->name()), 0);
}

QCOMPOSITOR_TEST_MAIN(ScreenPoolTest)

#include "screenpooltest.moc"
