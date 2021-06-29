/*
SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <QObject>

#include <QDir>
#include <QScreen>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include "../screenpool.h"

class ScreenPoolTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testScreenInsertion();
    void testPrimarySwap();

private:
    ScreenPool *m_screenPool;
};

void ScreenPoolTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

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
    int firstScreen = 0;
    if (QGuiApplication::primaryScreen()) {
        ++firstScreen;
    }
    qWarning() << "Known ids" << m_screenPool->knownIds();
    m_screenPool->insertScreenMapping(firstScreen, QStringLiteral("FAKE-0"));
    QCOMPARE(m_screenPool->knownIds().count(), firstScreen + 1);
    QCOMPARE(m_screenPool->connector(firstScreen), QStringLiteral("FAKE-0"));
    QCOMPARE(m_screenPool->id(QStringLiteral("FAKE-0")), firstScreen);

    qWarning() << "Known ids" << m_screenPool->knownIds();
    m_screenPool->insertScreenMapping(firstScreen + 1, QStringLiteral("FAKE-1"));
    QCOMPARE(m_screenPool->knownIds().count(), firstScreen + 2);
    QCOMPARE(m_screenPool->connector(firstScreen + 1), QStringLiteral("FAKE-1"));
    QCOMPARE(m_screenPool->id(QStringLiteral("FAKE-1")), firstScreen + 1);
}

void ScreenPoolTest::testPrimarySwap()
{
    const QString oldPrimary = QGuiApplication::primaryScreen()->name();
    QCOMPARE(m_screenPool->primaryConnector(), oldPrimary);
    const int oldScreenCount = m_screenPool->knownIds().count();
    const int oldIdOfFake1 = m_screenPool->id(QStringLiteral("FAKE-1"));
    m_screenPool->setPrimaryConnector(QStringLiteral("FAKE-1"));

    QCOMPARE(m_screenPool->knownIds().count(), oldScreenCount);

    QCOMPARE(m_screenPool->connector(0), QStringLiteral("FAKE-1"));
    QCOMPARE(m_screenPool->id(QStringLiteral("FAKE-1")), 0);

    QCOMPARE(m_screenPool->connector(oldIdOfFake1), oldPrimary);
    QCOMPARE(m_screenPool->id(oldPrimary), oldIdOfFake1);
}

QTEST_MAIN(ScreenPoolTest)

#include "screenpooltest.moc"
