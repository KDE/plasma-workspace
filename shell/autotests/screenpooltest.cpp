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

#include <wayland-server.h>

#include "../screenpool.h"

class ScreenPoolTest : public QObject
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
    struct wl_display *m_display;
};

void ScreenPoolTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    m_display = wl_display_create();
    QVERIFY(m_display);

    const char *socket = wl_display_add_socket_auto(m_display);
    QVERIFY(socket);

    qDebug() << "Running Wayland display on" << socket;
    wl_display_run(m_display); // FIXME: thould this be in a thread?

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

    wl_display_destroy(m_display);
}

void ScreenPoolTest::testScreenInsertion()
{
    QSignalSpy addedSpy(m_screenPool, SIGNAL(screenAdded(QScreen *)));

    // Add a new output
    addedSpy.wait();
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

QTEST_MAIN(ScreenPoolTest)

#include "screenpooltest.moc"
