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
        // NOTE: assumes that when a screen is added it will already have the final geometry
        auto *newOutput = add<Output>(data);
    });

    addedSpy.wait();
    QCOMPARE(QGuiApplication::screens().size(), 2);
    QCOMPARE(addedSpy.size(), 1);

    QScreen *newScreen = addedSpy.takeFirst().at(0).value<QScreen *>();
    QCOMPARE(newScreen->geometry(), QRect(1920, 0, 1920, 1080));
    //...
}

void ScreenPoolTest::testPrimarySwap()
{
    QSignalSpy primaryChangeSpy(m_screenPool, SIGNAL(primaryScreenChanged(QScreen *, QScreen *)));
    // Set a primary screen

    primaryChangeSpy.wait();
}

void ScreenPoolTest::testScreenRemoval()
{
    QSignalSpy removedSpy(m_screenPool, SIGNAL(screenRemoved(QScreen *)));

    // Add a new output
    removedSpy.wait();
    QScreen *removedScreen = removedSpy.takeFirst().at(0).value<QScreen *>();
}

QCOMPOSITOR_TEST_MAIN(ScreenPoolTest)

#include "screenpooltest.moc"
