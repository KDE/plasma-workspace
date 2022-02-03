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
    void testPrimarySwap();
    void testScreenRemoval();

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
    QCOMPARE(newScreen->geometry(), QRect(1920, 0, 1920, 1080));
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

void ScreenPoolTest::testScreenRemoval()
{
    QSignalSpy primaryChangeSpy(m_screenPool, SIGNAL(primaryScreenChanged(QScreen *, QScreen *)));
    QSignalSpy removedSpy(m_screenPool, SIGNAL(screenRemoved(QScreen *)));

    // Remove an output
    exec([=] {
        // NOTE: Assume the server will always do the right thing to change the primary screen before deleting one
        primaryOutput()->setPrimaryOutputName("WL-1");
        wl_display_flush_clients(m_display);
        remove(output(1));
    });
    primaryChangeSpy.wait();
    QCOMPARE(primaryChangeSpy.size(), 1);
    QScreen *oldPrimary = primaryChangeSpy[0].at(0).value<QScreen *>();
    QScreen *newPrimary = primaryChangeSpy[0].at(1).value<QScreen *>();
    QCOMPARE(newPrimary->name(), QStringLiteral("WL-1"));
    QCOMPARE(newPrimary->geometry(), QRect(0, 0, 1920, 1080));
    QCOMPARE(oldPrimary->name(), QStringLiteral("WL-2"));
    QCOMPARE(oldPrimary->geometry(), QRect(1920, 0, 1920, 1080));

    // NOTE: we can neither access the data of removedSpy nor oldPrimary because at this point will be dangling
    removedSpy.wait();
    QCOMPARE(QGuiApplication::screens().size(), 1);
    QCOMPARE(m_screenPool->screens().size(), 1);
    QScreen *lastScreen = m_screenPool->screens().first();
    QCOMPARE(lastScreen, newPrimary);
}

QCOMPOSITOR_TEST_MAIN(ScreenPoolTest)

#include "screenpooltest.moc"
