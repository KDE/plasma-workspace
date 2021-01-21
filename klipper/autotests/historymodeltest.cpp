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
#include "../historymodel.h"
#include "../historyimageitem.h"
#include "../historystringitem.h"
#include "../historyurlitem.h"

#include <QAbstractItemModelTester>
#include <QtTest>

class HistoryModelTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testSetMaxSize();
    void testInsertRemove();
    void testClear();
    void testIndexOf();
    void testType_data();
    void testType();
};

void HistoryModelTest::testSetMaxSize()
{
    QScopedPointer<HistoryModel> history(new HistoryModel(nullptr));
    QScopedPointer<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(history.data()));

    QCOMPARE(history->rowCount(), 0);
    QCOMPARE(history->maxSize(), 0);

    // insert an item - should still be empty
    history->insert(QSharedPointer<HistoryItem>(new HistoryStringItem(QStringLiteral("foo"))));
    QCOMPARE(history->rowCount(), 0);

    // now it should insert again
    history->setMaxSize(1);
    QCOMPARE(history->maxSize(), 1);
    history->insert(QSharedPointer<HistoryItem>(new HistoryStringItem(QStringLiteral("foo"))));
    QCOMPARE(history->rowCount(), 1);

    // insert another item, foo should get removed
    history->insert(QSharedPointer<HistoryItem>(new HistoryStringItem(QStringLiteral("bar"))));
    QCOMPARE(history->rowCount(), 1);
    QCOMPARE(history->data(history->index(0, 0)).toString(), QLatin1String("bar"));

    // I don't trust the model, add more items
    history->setMaxSize(10);
    history->insert(QSharedPointer<HistoryItem>(new HistoryStringItem(QStringLiteral("foo"))));
    history->insert(QSharedPointer<HistoryItem>(new HistoryStringItem(QStringLiteral("foobar"))));
    QCOMPARE(history->rowCount(), 3);
    QCOMPARE(history->data(history->index(0, 0)).toString(), QLatin1String("foobar"));
    QCOMPARE(history->data(history->index(1, 0)).toString(), QLatin1String("foo"));
    QCOMPARE(history->data(history->index(2, 0)).toString(), QLatin1String("bar"));

    // setting to 0 should clear again
    history->setMaxSize(0);
    QCOMPARE(history->maxSize(), 0);
    QCOMPARE(history->rowCount(), 0);
}

void HistoryModelTest::testInsertRemove()
{
    QScopedPointer<HistoryModel> history(new HistoryModel(nullptr));
    QScopedPointer<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(history.data()));
    history->setMaxSize(10);
    QCOMPARE(history->rowCount(), 0);

    const QString fooText = QStringLiteral("foo");
    const QString barText = QStringLiteral("bar");
    const QString fooBarText = QStringLiteral("foobar");
    const QByteArray fooUuid = QCryptographicHash::hash(fooText.toUtf8(), QCryptographicHash::Sha1);
    const QByteArray barUuid = QCryptographicHash::hash(barText.toUtf8(), QCryptographicHash::Sha1);
    const QByteArray foobarUuid = QCryptographicHash::hash(fooBarText.toUtf8(), QCryptographicHash::Sha1);

    // let's insert a few items
    history->insert(QSharedPointer<HistoryItem>(new HistoryStringItem(fooText)));
    QModelIndex index = history->index(0, 0);
    QCOMPARE(index.data().toString(), fooText);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->next_uuid(), index.data(Qt::UserRole + 1).toByteArray());
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->previous_uuid(), index.data(Qt::UserRole + 1).toByteArray());

    history->insert(QSharedPointer<HistoryItem>(new HistoryStringItem(barText)));
    QCOMPARE(index.data().toString(), barText);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->next_uuid(), fooUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->previous_uuid(), fooUuid);
    index = history->indexOf(fooUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->next_uuid(), barUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->previous_uuid(), barUuid);

    history->insert(QSharedPointer<HistoryItem>(new HistoryStringItem(fooBarText)));
    QCOMPARE(history->data(history->index(0, 0)).toString(), fooBarText);
    index = history->index(0, 0);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->next_uuid(), barUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->previous_uuid(), fooUuid);
    index = history->indexOf(fooUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->next_uuid(), foobarUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->previous_uuid(), barUuid);
    index = history->indexOf(barUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->next_uuid(), fooUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->previous_uuid(), foobarUuid);

    // insert one again - it should be moved to top
    history->insert(QSharedPointer<HistoryItem>(new HistoryStringItem(barText)));
    QCOMPARE(history->data(history->index(0, 0)).toString(), barText);
    index = history->index(0, 0);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->next_uuid(), foobarUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->previous_uuid(), fooUuid);
    index = history->indexOf(fooUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->next_uuid(), barUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->previous_uuid(), foobarUuid);
    index = history->indexOf(foobarUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->next_uuid(), fooUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->previous_uuid(), barUuid);

    // move one to top using the slot
    // already on top, shouldn't change anything
    history->moveToTop(barUuid);
    QCOMPARE(history->data(history->index(0, 0)).toString(), barText);
    index = history->index(0, 0);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->next_uuid(), foobarUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->previous_uuid(), fooUuid);
    index = history->indexOf(fooUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->next_uuid(), barUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->previous_uuid(), foobarUuid);
    index = history->indexOf(foobarUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->next_uuid(), fooUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->previous_uuid(), barUuid);

    // another one should change, though
    history->moveToTop(foobarUuid);
    QCOMPARE(history->data(history->index(0, 0)).toString(), fooBarText);
    index = history->index(0, 0);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->next_uuid(), barUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->previous_uuid(), fooUuid);
    index = history->indexOf(fooUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->next_uuid(), foobarUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->previous_uuid(), barUuid);
    index = history->indexOf(barUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->next_uuid(), fooUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->previous_uuid(), foobarUuid);

    // remove them again
    QVERIFY(history->remove(foobarUuid));
    QCOMPARE(history->data(history->index(0, 0)).toString(), barText);
    index = history->index(0, 0);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->next_uuid(), fooUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->previous_uuid(), fooUuid);
    index = history->indexOf(fooUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->next_uuid(), barUuid);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->previous_uuid(), barUuid);

    QVERIFY(history->remove(barUuid));
    QCOMPARE(history->data(history->index(0, 0)).toString(), fooText);
    index = history->index(0, 0);
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->next_uuid(), index.data(Qt::UserRole + 1).toByteArray());
    QCOMPARE(index.data(Qt::UserRole).value<HistoryItemConstPtr>()->previous_uuid(), index.data(Qt::UserRole + 1).toByteArray());

    QVERIFY(history->remove(fooUuid));
    QCOMPARE(history->rowCount(), 0);
}

void HistoryModelTest::testClear()
{
    QScopedPointer<HistoryModel> history(new HistoryModel(nullptr));
    QScopedPointer<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(history.data()));
    history->setMaxSize(10);
    QCOMPARE(history->rowCount(), 0);

    history->clear();
    QCOMPARE(history->rowCount(), 0);

    // insert some items
    history->insert(QSharedPointer<HistoryItem>(new HistoryStringItem(QStringLiteral("foo"))));
    history->insert(QSharedPointer<HistoryItem>(new HistoryStringItem(QStringLiteral("bar"))));
    history->insert(QSharedPointer<HistoryItem>(new HistoryStringItem(QStringLiteral("foobar"))));
    history->moveToTop(QCryptographicHash::hash(QByteArrayLiteral("bar"), QCryptographicHash::Sha1));
    QCOMPARE(history->rowCount(), 3);

    // and clear
    history->clear();
    QCOMPARE(history->rowCount(), 0);
}

void HistoryModelTest::testIndexOf()
{
    QScopedPointer<HistoryModel> history(new HistoryModel(nullptr));
    QScopedPointer<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(history.data()));
    history->setMaxSize(10);
    QCOMPARE(history->rowCount(), 0);
    QVERIFY(!history->indexOf(QByteArrayLiteral("whatever")).isValid());
    QVERIFY(!history->indexOf(QByteArray()).isValid());

    // insert some items
    history->insert(QSharedPointer<HistoryItem>(new HistoryStringItem(QStringLiteral("foo"))));
    QVERIFY(!history->indexOf(QByteArrayLiteral("whatever")).isValid());
    QVERIFY(!history->indexOf(QByteArray()).isValid());
    const QByteArray fooUuid = QCryptographicHash::hash(QByteArrayLiteral("foo"), QCryptographicHash::Sha1);
    QVERIFY(history->indexOf(fooUuid).isValid());
    QCOMPARE(history->indexOf(fooUuid).data(Qt::UserRole + 1).toByteArray(), fooUuid);

    history->clear();
    QVERIFY(!history->indexOf(fooUuid).isValid());
}

void HistoryModelTest::testType_data()
{
    QTest::addColumn<HistoryItem *>("item");
    QTest::addColumn<HistoryItemType>("expectedType");

    HistoryItem *item = new HistoryStringItem(QStringLiteral("foo"));
    QTest::newRow("text") << item << HistoryItemType::Text;
    item = new HistoryImageItem(QPixmap());
    QTest::newRow("image") << item << HistoryItemType::Image;
    item = new HistoryURLItem(QList<QUrl>(), KUrlMimeData::MetaDataMap(), false);
    QTest::newRow("url") << item << HistoryItemType::Url;
}

void HistoryModelTest::testType()
{
    QScopedPointer<HistoryModel> history(new HistoryModel(nullptr));
    QScopedPointer<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(history.data()));
    history->setMaxSize(10);
    QCOMPARE(history->rowCount(), 0);

    QFETCH(HistoryItem *, item);
    QFETCH(HistoryItemType, expectedType);
    history->insert(QSharedPointer<HistoryItem>(item));
    QCOMPARE(history->index(0).data(Qt::UserRole + 2).value<HistoryItemType>(), expectedType);
}

QTEST_MAIN(HistoryModelTest)
#include "historymodeltest.moc"
