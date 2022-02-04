/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <QObject>

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
    void testSecondScreenRemoval();
    void testThirdScreenRemoval();

private:
    ScreenPool *m_screenPool;
};

void ScreenPoolTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    qRegisterMetaType<QScreen *>();
    m_config.autoConfigure = true;
    m_config.autoEnter = false;

    KConfigGroup cg(KSharedConfig::openConfig(), QStringLiteral("ScreenConnectors"));
    cg.deleteGroup();
    cg.sync();
    m_screenPool = new ScreenPool(KSharedConfig::openConfig(), this);
    m_screenPool->load();

    QTRY_COMPARE(QGuiApplication::screens().size(), 1);
    QCOMPARE(m_screenPool->screens().size(), 1);
    QCOMPARE(QGuiApplication::screens().first()->name(), QStringLiteral("WL-1"));
    QCOMPARE(QGuiApplication::primaryScreen(), QGuiApplication::screens().first());
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
}

void ScreenPoolTest::testMoveOutOfRedundant()
{
    QSignalSpy addedSpy(m_screenPool, SIGNAL(screenAdded(QScreen *)));

    exec([=] {
        auto *out = output(2);
        auto xdgOut = xdgOutput(out);
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
        auto xdgOut = xdgOutput(out);
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
}

void ScreenPoolTest::testSecondScreenRemoval()
{
    QSignalSpy addedSpy(m_screenPool, SIGNAL(screenAdded(QScreen *)));
    QSignalSpy primaryChangeSpy(m_screenPool, SIGNAL(primaryScreenChanged(QScreen *, QScreen *)));
    QSignalSpy removedSpy(m_screenPool, SIGNAL(screenRemoved(QScreen *)));

    // Remove an output
    exec([=] {
        // NOTE: Assume the server will always do the right thing to change the primary screen before deleting one
        primaryOutput()->setPrimaryOutputName("WL-1");
        wl_display_flush_clients(m_display);
        remove(output(1));
    });

    // Removing the primary screen, will change a primaryChange signal beforehand
    primaryChangeSpy.wait();
    QCOMPARE(primaryChangeSpy.size(), 1);
    QScreen *oldPrimary = primaryChangeSpy[0].at(0).value<QScreen *>();
    QScreen *newPrimary = primaryChangeSpy[0].at(1).value<QScreen *>();
    QCOMPARE(newPrimary->name(), QStringLiteral("WL-1"));
    QCOMPARE(newPrimary->geometry(), QRect(0, 0, 1920, 1080));
    QCOMPARE(oldPrimary->name(), QStringLiteral("WL-2"));
    QCOMPARE(oldPrimary->geometry(), QRect(1920, 0, 1920, 1080));

    // NOTE: we can neither access the data of removedSpy nor oldPrimary because at this point will be dangling
    QTRY_COMPARE(removedSpy.size(), 1);
    QTRY_COMPARE(addedSpy.size(), 1);

    QCOMPARE(QGuiApplication::screens().size(), 2);
    QCOMPARE(m_screenPool->screens().size(), 2);
    QScreen *firstScreen = m_screenPool->screens().first();
    QCOMPARE(firstScreen, newPrimary);

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
        remove(output(1));
    });

    // NOTE: we can neither access the data of removedSpy nor oldPrimary because at this point will be dangling
    removedSpy.wait();
    QCOMPARE(QGuiApplication::screens().size(), 1);
    QCOMPARE(m_screenPool->screens().size(), 1);
    QScreen *lastScreen = m_screenPool->screens().first();
}

QCOMPOSITOR_TEST_MAIN(ScreenPoolTest)

#include "screenpooltest.moc"
