/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../historycycler.h"
#include "../historymodel.h"
#include "../historystringitem.h"
// Qt
#include <QObject>
#include <QSignalSpy>
#include <QTest>

class HistoryCyclerTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testCycle();
};

void HistoryCyclerTest::testCycle()
{
    auto model = HistoryModel::self();
    std::unique_ptr<HistoryCycler> history(new HistoryCycler(nullptr));
    QSignalSpy changedSpy(model.get(), &HistoryModel::changed);
    model->setMaxSize(10);
    QVERIFY(!history->nextInCycle());
    QVERIFY(!history->prevInCycle());

    const QString fooText = QStringLiteral("foo");
    const QString barText = QStringLiteral("bar");
    const QString fooBarText = QStringLiteral("foobar");
    const QByteArray fooUuid = QCryptographicHash::hash(fooText.toUtf8(), QCryptographicHash::Sha1);
    const QByteArray barUuid = QCryptographicHash::hash(barText.toUtf8(), QCryptographicHash::Sha1);
    const QByteArray foobarUuid = QCryptographicHash::hash(fooBarText.toUtf8(), QCryptographicHash::Sha1);

    model->insert(HistoryItemPtr(new HistoryStringItem(fooText)));
    QCOMPARE(changedSpy.size(), 1);
    changedSpy.clear();
    QVERIFY(!history->nextInCycle());
    QVERIFY(!history->prevInCycle());
    // cycling to next shouldn't change anything
    history->cycleNext();
    QVERIFY(changedSpy.isEmpty());
    QVERIFY(!history->nextInCycle());
    QVERIFY(!history->prevInCycle());
    // cycling to previous shouldn't change anything
    history->cyclePrev();
    QVERIFY(changedSpy.isEmpty());
    QVERIFY(!history->nextInCycle());
    QVERIFY(!history->prevInCycle());

    // insert more items
    model->insert(HistoryItemPtr(new HistoryStringItem(barText)));
    QCOMPARE(changedSpy.size(), 1);
    changedSpy.clear();
    QCOMPARE(history->nextInCycle()->uuid(), fooUuid);
    QVERIFY(!history->prevInCycle());
    // cycle to next
    history->cycleNext();
    QCOMPARE(changedSpy.size(), 1);
    changedSpy.clear();
    QCOMPARE(model->first()->uuid(), fooUuid);
    QVERIFY(!history->nextInCycle());
    QCOMPARE(history->prevInCycle()->uuid(), barUuid);
    // there are no more next
    history->cycleNext();
    QVERIFY(changedSpy.isEmpty());
    // cycle to prev should restore previous state
    history->cyclePrev();
    QCOMPARE(changedSpy.size(), 1);
    changedSpy.clear();
    QCOMPARE(model->first()->uuid(), barUuid);
    QCOMPARE(history->nextInCycle()->uuid(), fooUuid);
    QVERIFY(!history->prevInCycle());
    // there are no more prev
    history->cyclePrev();
    QVERIFY(changedSpy.isEmpty());

    // insert a third item
    model->insert(HistoryItemPtr(new HistoryStringItem(fooBarText)));
    QCOMPARE(changedSpy.size(), 1);
    changedSpy.clear();
    QCOMPARE(history->nextInCycle()->uuid(), barUuid);
    QVERIFY(!history->prevInCycle());
    // cycle to next
    history->cycleNext();
    QCOMPARE(changedSpy.size(), 1);
    changedSpy.clear();
    QCOMPARE(model->first()->uuid(), barUuid);
    QCOMPARE(history->nextInCycle()->uuid(), fooUuid);
    QCOMPARE(history->prevInCycle()->uuid(), foobarUuid);
    // cycle to next (should be last)
    history->cycleNext();
    QCOMPARE(changedSpy.size(), 1);
    changedSpy.clear();
    QCOMPARE(model->first()->uuid(), fooUuid);
    QVERIFY(!history->nextInCycle());
    QCOMPARE(history->prevInCycle()->uuid(), barUuid);
    // there are no more next
    history->cycleNext();
    QVERIFY(changedSpy.isEmpty());
    // and back again
    history->cyclePrev();
    QCOMPARE(changedSpy.size(), 1);
    changedSpy.clear();
    QCOMPARE(model->first()->uuid(), barUuid);
    QCOMPARE(history->nextInCycle()->uuid(), fooUuid);
    QCOMPARE(history->prevInCycle()->uuid(), foobarUuid);
    // one more
    history->cyclePrev();
    QCOMPARE(changedSpy.size(), 1);
    changedSpy.clear();
    QCOMPARE(model->first()->uuid(), foobarUuid);
    QCOMPARE(history->nextInCycle()->uuid(), barUuid);
    QVERIFY(!history->prevInCycle());
    // there are no more prev
    history->cyclePrev();
    QVERIFY(changedSpy.isEmpty());
}

QTEST_MAIN(HistoryCyclerTest)
#include "historycyclertest.moc"
