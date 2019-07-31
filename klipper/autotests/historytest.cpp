/********************************************************************
This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "../history.h"
#include "../historystringitem.h"
// Qt
#include <QtTest>
#include <QSignalSpy>
#include <QObject>

class HistoryTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testSetMaxSize();
    void testInsertRemove();
    void testClear();
    void testFind();
    void testCycle();
};

void HistoryTest::testSetMaxSize()
{
    QScopedPointer<History> history(new History(nullptr));
    QSignalSpy changedSpy(history.data(), &History::changed);
    QSignalSpy topSpy(history.data(), &History::topChanged);
    QVERIFY(history->empty());
    QCOMPARE(history->maxSize(), 0u);
    QVERIFY(changedSpy.isEmpty());
    QVERIFY(topSpy.isEmpty());

    // insert an item - should still be empty
    history->insert(HistoryItemPtr(new HistoryStringItem(QStringLiteral("foo"))));
    QVERIFY(history->empty());
    QVERIFY(changedSpy.isEmpty());
    QVERIFY(topSpy.isEmpty());

    // now it should insert again
    history->setMaxSize(1);
    QCOMPARE(history->maxSize(), 1u);
    history->insert(HistoryItemPtr(new HistoryStringItem(QStringLiteral("foo"))));
    QVERIFY(!history->empty());
    QCOMPARE(changedSpy.size(), 1);
    changedSpy.clear();
    QVERIFY(changedSpy.isEmpty());
    QCOMPARE(topSpy.size(), 1);
    topSpy.clear();
    QVERIFY(topSpy.isEmpty());

    // insert another item, foo should get removed
    history->insert(HistoryItemPtr(new HistoryStringItem(QStringLiteral("bar"))));
    QCOMPARE(history->first()->text(), QLatin1String("bar"));
    QCOMPARE(history->first()->next_uuid(), history->first()->uuid());
    QEXPECT_FAIL("", "Item is first inserted, then removed, changed signal gets emitted, although not changed", Continue);
    QCOMPARE(topSpy.size(), 1);
    topSpy.clear();
    QVERIFY(topSpy.isEmpty());
    // two changed events - once added, once removed
    QCOMPARE(changedSpy.size(), 2);
    changedSpy.clear();
    QVERIFY(changedSpy.isEmpty());

    // setting to 0 should clear again
    history->setMaxSize(0);
    QCOMPARE(history->maxSize(), 0u);
    QVERIFY(history->empty());
    QCOMPARE(changedSpy.size(), 1);
    QCOMPARE(topSpy.size(), 1);
}

void HistoryTest::testInsertRemove()
{
    QScopedPointer<History> history(new History(nullptr));
    QSignalSpy topSpy(history.data(), &History::topChanged);
    QSignalSpy topUserSelectedSpy(history.data(), &History::topIsUserSelectedSet);

    history->setMaxSize(10);
    QVERIFY(history->empty());
    QVERIFY(!history->topIsUserSelected());

    const QString fooText = QStringLiteral("foo");
    const QString barText = QStringLiteral("bar");
    const QString fooBarText = QStringLiteral("foobar");
    const QByteArray fooUuid = QCryptographicHash::hash(fooText.toUtf8(), QCryptographicHash::Sha1);
    const QByteArray barUuid = QCryptographicHash::hash(barText.toUtf8(), QCryptographicHash::Sha1);
    const QByteArray foobarUuid = QCryptographicHash::hash(fooBarText.toUtf8(), QCryptographicHash::Sha1);

    // let's insert a few items
    history->insert(HistoryItemPtr(new HistoryStringItem(fooText)));
    QVERIFY(!history->topIsUserSelected());
    QCOMPARE(history->first()->text(), fooText);
    QCOMPARE(history->first()->next_uuid(), history->first()->uuid());
    QCOMPARE(history->first()->previous_uuid(), history->first()->uuid());
    QCOMPARE(topSpy.size(), 1);
    topSpy.clear();
    QVERIFY(topSpy.isEmpty());

    history->insert(HistoryItemPtr(new HistoryStringItem(barText)));
    QVERIFY(!history->topIsUserSelected());
    QCOMPARE(history->first()->text(), barText);
    QCOMPARE(history->first()->next_uuid(), fooUuid);
    QCOMPARE(history->first()->previous_uuid(), fooUuid);
    QCOMPARE(history->find(fooUuid)->next_uuid(), barUuid);
    QCOMPARE(history->find(fooUuid)->previous_uuid(), barUuid);
    QCOMPARE(topSpy.size(), 1);
    topSpy.clear();
    QVERIFY(topSpy.isEmpty());

    history->insert(HistoryItemPtr(new HistoryStringItem(fooBarText)));
    QVERIFY(!history->topIsUserSelected());
    QCOMPARE(history->first()->text(), fooBarText);
    QCOMPARE(history->first()->next_uuid(), barUuid);
    QCOMPARE(history->first()->previous_uuid(), fooUuid);
    QCOMPARE(history->find(fooUuid)->next_uuid(), foobarUuid);
    QCOMPARE(history->find(fooUuid)->previous_uuid(), barUuid);
    QCOMPARE(history->find(barUuid)->next_uuid(), fooUuid);
    QCOMPARE(history->find(barUuid)->previous_uuid(), foobarUuid);
    QCOMPARE(topSpy.size(), 1);
    topSpy.clear();
    QVERIFY(topSpy.isEmpty());

    // insert one again - it should be moved to top
    history->insert(HistoryItemPtr(new HistoryStringItem(barText)));
    QVERIFY(!history->topIsUserSelected());
    QCOMPARE(history->first()->text(), barText);
    QCOMPARE(history->first()->next_uuid(), foobarUuid);
    QCOMPARE(history->first()->previous_uuid(), fooUuid);
    QCOMPARE(history->find(fooUuid)->next_uuid(), barUuid);
    QCOMPARE(history->find(fooUuid)->previous_uuid(), foobarUuid);
    QCOMPARE(history->find(foobarUuid)->next_uuid(), fooUuid);
    QCOMPARE(history->find(foobarUuid)->previous_uuid(), barUuid);
    QCOMPARE(topSpy.size(), 1);
    topSpy.clear();
    QVERIFY(topSpy.isEmpty());

    // move one to top using the slot
    // as well as the top changing, topIsUserSelected will also be signalled to show that it got re-selected
    history->slotMoveToTop(barUuid);
    QVERIFY(history->topIsUserSelected());
    QCOMPARE(history->first()->text(), barText);
    QCOMPARE(history->first()->next_uuid(), foobarUuid);
    QCOMPARE(history->first()->previous_uuid(), fooUuid);
    QCOMPARE(history->find(fooUuid)->next_uuid(), barUuid);
    QCOMPARE(history->find(fooUuid)->previous_uuid(), foobarUuid);
    QCOMPARE(history->find(foobarUuid)->next_uuid(), fooUuid);
    QCOMPARE(history->find(foobarUuid)->previous_uuid(), barUuid);
    QCOMPARE(topSpy.size(), 1);
    topSpy.clear();
    QVERIFY(topSpy.isEmpty());
    QCOMPARE(topUserSelectedSpy.size(), 1);
    topUserSelectedSpy.clear();
    QVERIFY(topUserSelectedSpy.isEmpty());

    // another one should change, though
    history->slotMoveToTop(foobarUuid);
    QVERIFY(history->topIsUserSelected());
    QCOMPARE(history->first()->text(), fooBarText);
    QCOMPARE(history->first()->next_uuid(), barUuid);
    QCOMPARE(history->first()->previous_uuid(), fooUuid);
    QCOMPARE(history->find(fooUuid)->next_uuid(), foobarUuid);
    QCOMPARE(history->find(fooUuid)->previous_uuid(), barUuid);
    QCOMPARE(history->find(barUuid)->next_uuid(), fooUuid);
    QCOMPARE(history->find(barUuid)->previous_uuid(), foobarUuid);
    QCOMPARE(topSpy.size(), 1);
    topSpy.clear();
    QVERIFY(topSpy.isEmpty());

    // remove them again
    history->remove(history->find(foobarUuid));
    QVERIFY(!history->topIsUserSelected());
    QCOMPARE(history->first()->text(), barText);
    QCOMPARE(history->first()->next_uuid(), fooUuid);
    QCOMPARE(history->first()->previous_uuid(), fooUuid);
    QCOMPARE(history->find(fooUuid)->next_uuid(), barUuid);
    QCOMPARE(history->find(fooUuid)->previous_uuid(), barUuid);
    QCOMPARE(topSpy.size(), 1);
    topSpy.clear();
    QVERIFY(topSpy.isEmpty());

    history->remove(history->find(barUuid));
    QVERIFY(!history->topIsUserSelected());
    QCOMPARE(history->first()->text(), fooText);
    QCOMPARE(history->first()->next_uuid(), history->first()->uuid());
    QCOMPARE(history->first()->previous_uuid(), history->first()->uuid());
    QCOMPARE(topSpy.size(), 1);
    topSpy.clear();
    QVERIFY(topSpy.isEmpty());

    history->remove(history->find(fooUuid));
    QVERIFY(!history->topIsUserSelected());
    QVERIFY(!history->first());
    QVERIFY(history->empty());
    QCOMPARE(topSpy.size(), 1);
}

void HistoryTest::testClear()
{
    QScopedPointer<History> history(new History(nullptr));
    history->setMaxSize(10);
    QVERIFY(history->empty());
    QVERIFY(!history->topIsUserSelected());

    history->slotClear();
    QVERIFY(history->empty());

    // insert some items
    history->insert(HistoryItemPtr(new HistoryStringItem(QStringLiteral("foo"))));
    history->insert(HistoryItemPtr(new HistoryStringItem(QStringLiteral("bar"))));
    history->insert(HistoryItemPtr(new HistoryStringItem(QStringLiteral("foobar"))));
    history->slotMoveToTop(QCryptographicHash::hash(QByteArrayLiteral("bar"), QCryptographicHash::Sha1));
    QVERIFY(!history->empty());
    QVERIFY(history->topIsUserSelected());
    QVERIFY(history->first());

    // and clear
    QSignalSpy topSpy(history.data(), &History::topChanged);
    QVERIFY(topSpy.isEmpty());
    history->slotClear();
    QVERIFY(history->empty());
    QVERIFY(!history->first());
    QVERIFY(!history->topIsUserSelected());
    QVERIFY(!topSpy.isEmpty());
}

void HistoryTest::testFind()
{
    QScopedPointer<History> history(new History(nullptr));
    history->setMaxSize(10);
    QVERIFY(history->empty());
    QVERIFY(!history->find(QByteArrayLiteral("whatever")));
    QVERIFY(!history->find(QByteArray()));

    // insert some items
    history->insert(HistoryItemPtr(new HistoryStringItem(QStringLiteral("foo"))));
    QVERIFY(!history->find(QByteArrayLiteral("whatever")));
    QVERIFY(!history->find(QByteArray()));
    const QByteArray fooUuid = QCryptographicHash::hash(QByteArrayLiteral("foo"), QCryptographicHash::Sha1);
    QCOMPARE(history->find(fooUuid)->uuid(), fooUuid);

    history->slotClear();
    QVERIFY(!history->find(fooUuid));
}

void HistoryTest::testCycle()
{
    QScopedPointer<History> history(new History(nullptr));
    QSignalSpy topSpy(history.data(), &History::topChanged);
    history->setMaxSize(10);
    QVERIFY(!history->nextInCycle());
    QVERIFY(!history->prevInCycle());

    const QString fooText = QStringLiteral("foo");
    const QString barText = QStringLiteral("bar");
    const QString fooBarText = QStringLiteral("foobar");
    const QByteArray fooUuid = QCryptographicHash::hash(fooText.toUtf8(), QCryptographicHash::Sha1);
    const QByteArray barUuid = QCryptographicHash::hash(barText.toUtf8(), QCryptographicHash::Sha1);
    const QByteArray foobarUuid = QCryptographicHash::hash(fooBarText.toUtf8(), QCryptographicHash::Sha1);

    history->insert(HistoryItemPtr(new HistoryStringItem(fooText)));
    QCOMPARE(topSpy.size(), 1);
    topSpy.clear();
    QVERIFY(!history->nextInCycle());
    QVERIFY(!history->prevInCycle());
    // cycling to next shouldn't change anything
    history->cycleNext();
    QVERIFY(topSpy.isEmpty());
    QVERIFY(!history->nextInCycle());
    QVERIFY(!history->prevInCycle());
    // cycling to previous shouldn't change anything
    history->cyclePrev();
    QVERIFY(topSpy.isEmpty());
    QVERIFY(!history->nextInCycle());
    QVERIFY(!history->prevInCycle());

    // insert more items
    history->insert(HistoryItemPtr(new HistoryStringItem(barText)));
    QCOMPARE(topSpy.size(), 1);
    topSpy.clear();
    QCOMPARE(history->nextInCycle(), history->find(fooUuid));
    QVERIFY(!history->prevInCycle());
    // cycle to next
    history->cycleNext();
    QCOMPARE(topSpy.size(), 1);
    topSpy.clear();
    QCOMPARE(history->first(), history->find(fooUuid));
    QVERIFY(!history->nextInCycle());
    QCOMPARE(history->prevInCycle(), history->find(barUuid));
    // there are no more next
    history->cycleNext();
    QVERIFY(topSpy.isEmpty());
    // cycle to prev should restore previous state
    history->cyclePrev();
    QCOMPARE(topSpy.size(), 1);
    topSpy.clear();
    QCOMPARE(history->first(), history->find(barUuid));
    QCOMPARE(history->nextInCycle(), history->find(fooUuid));
    QVERIFY(!history->prevInCycle());
    // there are no more prev
    history->cyclePrev();
    QVERIFY(topSpy.isEmpty());

    // insert a third item
    history->insert(HistoryItemPtr(new HistoryStringItem(fooBarText)));
    QCOMPARE(topSpy.size(), 1);
    topSpy.clear();
    QCOMPARE(history->nextInCycle(), history->find(barUuid));
    QVERIFY(!history->prevInCycle());
    // cycle to next
    history->cycleNext();
    QCOMPARE(topSpy.size(), 1);
    topSpy.clear();
    QCOMPARE(history->first(), history->find(barUuid));
    QCOMPARE(history->nextInCycle(), history->find(fooUuid));
    QCOMPARE(history->prevInCycle(), history->find(foobarUuid));
    // cycle to next (should be last)
    history->cycleNext();
    QCOMPARE(topSpy.size(), 1);
    topSpy.clear();
    QCOMPARE(history->first(), history->find(fooUuid));
    QVERIFY(!history->nextInCycle());
    QCOMPARE(history->prevInCycle(), history->find(barUuid));
    // there are no more next
    history->cycleNext();
    QVERIFY(topSpy.isEmpty());
    // and back again
    history->cyclePrev();
    QCOMPARE(topSpy.size(), 1);
    topSpy.clear();
    QCOMPARE(history->first(), history->find(barUuid));
    QCOMPARE(history->nextInCycle(), history->find(fooUuid));
    QCOMPARE(history->prevInCycle(), history->find(foobarUuid));
    // one more
    history->cyclePrev();
    QCOMPARE(topSpy.size(), 1);
    topSpy.clear();
    QCOMPARE(history->first(), history->find(foobarUuid));
    QCOMPARE(history->nextInCycle(), history->find(barUuid));
    QVERIFY(!history->prevInCycle());
    // there are no more prev
    history->cyclePrev();
    QVERIFY(topSpy.isEmpty());
}

QTEST_MAIN(HistoryTest)
#include "historytest.moc"
