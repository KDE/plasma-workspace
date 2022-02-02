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

class XdgOutputV1Compositor : public DefaultCompositor
{
public:
    explicit XdgOutputV1Compositor()
    {
        exec([this] {
            int version = 3; // version 3 of of unstable-v1
            add<XdgOutputManagerV1>(version);
        });
    }
    XdgOutputV1 *xdgOutput(int i = 0)
    {
        return get<XdgOutputManagerV1>()->getXdgOutput(output(i));
    }
};

class ScreenPoolTest : public QObject, XdgOutputV1Compositor
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

    KConfigGroup cg(KSharedConfig::openConfig(), QStringLiteral("ScreenConnectors"));
    cg.deleteGroup();
    cg.sync();
    m_screenPool = new ScreenPool(KSharedConfig::openConfig(), this);
    m_screenPool->load();
}

void ScreenPoolTest::cleanupTestCase()
{
    KConfigGroup cg(KSharedConfig::openConfig(), QStringLiteral("ScreenConnectors"));
    cg.deleteGroup();
    cg.sync();
}

void ScreenPoolTest::testScreenInsertion()
{
    QSignalSpy addedSpy(m_screenPool, SIGNAL(screenAdded(QScreen *)));

    // Add a new output
    exec([=] {
        auto *oldOutput = output(0);
        auto *newOutput = add<Output>();
        newOutput->m_data.mode.resolution = {1920, 1080};
        newOutput->m_data.position = {1920, 0};
        // Move the primary output to the right
        /* QPoint newPosition(newOutput->m_data.mode.resolution.width(), 0);
         Q_ASSERT(newPosition != initialPosition);
         oldOutput->m_data.position = newPosition;
         oldOutput->sendGeometry();
         oldOutput->sendDone();*/
    });

    QTRY_COMPARE(QGuiApplication::screens().size(), 2);
    addedSpy.wait();
    qWarning() << addedSpy << QGuiApplication::screens();
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
