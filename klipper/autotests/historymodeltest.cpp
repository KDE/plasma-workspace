/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../historymodel.h"
#include "../historyitem.h"
#include "systemclipboard.h"

#include <QAbstractItemModelTester>
#include <QStandardPaths>
#include <QTest>

class HistoryModelTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testSetMaxSize();
    void testInsertRemove();
    void testClear();
    void testIndexOf();
    void testType_data();
    void testType();
};

void HistoryModelTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void HistoryModelTest::testSetMaxSize()
{
    SystemClipboard::self()->clear();
    std::shared_ptr<HistoryModel> history = HistoryModel::self();
    std::unique_ptr<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(history.get()));
    history->setMaxSize(0);

    QCOMPARE(history->rowCount(), 0);
    QCOMPARE(history->maxSize(), 0); // Default value

    // insert an item - should still be empty
    history->insert(HistoryItem::create(QStringLiteral("foo")));
    QCOMPARE(history->rowCount(), 0);

    // now it should insert again
    history->setMaxSize(1);
    QCOMPARE(history->maxSize(), 1);
    history->insert(HistoryItem::create(QStringLiteral("foo")));
    QCOMPARE(history->rowCount(), 1);

    // insert another item, foo should get removed
    history->insert(HistoryItem::create(QStringLiteral("bar")));
    QCOMPARE(history->rowCount(), 1);
    QCOMPARE(history->data(history->index(0, 0)).toString(), QLatin1String("bar"));

    // I don't trust the model, add more items
    history->setMaxSize(10);
    history->insert(HistoryItem::create(QStringLiteral("foo")));
    history->insert(HistoryItem::create(QStringLiteral("foobar")));
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
    std::shared_ptr<HistoryModel> history = HistoryModel::self();
    std::unique_ptr<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(history.get()));
    history->setMaxSize(10);
    QCOMPARE(history->rowCount(), 0);

    const QString fooText = QStringLiteral("foo");
    const auto fooItem = HistoryItem::create(fooText);
    const auto fooUuid = fooItem->uuid();

    const QString barText = QStringLiteral("bar");
    const auto barItem = HistoryItem::create(barText);
    const auto barUuid = barItem->uuid();

    const QString foobarText = QStringLiteral("foobar");
    const auto foobarItem = HistoryItem::create(foobarText);
    const auto foobarUuid = foobarItem->uuid();

    // let's insert a few items
    history->insert(fooItem);
    QModelIndex index = history->index(0, 0);
    QCOMPARE(index.data().toString(), fooText);

    history->insert(barItem);
    QCOMPARE(index.data().toString(), barText);

    history->insert(foobarItem);
    QCOMPARE(history->data(history->index(0, 0)).toString(), foobarText);
    index = history->index(0, 0);

    // insert one again - it should be moved to top
    history->insert(HistoryItem::create(barText));
    QCOMPARE(history->data(history->index(0, 0)).toString(), barText);

    // move one to top using the slot
    // already on top, shouldn't change anything
    history->moveToTop(barUuid);
    QCOMPARE(history->data(history->index(0, 0)).toString(), barText);

    // another one should change, though
    history->moveToTop(foobarUuid);
    QCOMPARE(history->data(history->index(0, 0)).toString(), foobarText);

    // remove them again
    QVERIFY(history->remove(foobarUuid));
    QCOMPARE(history->data(history->index(0, 0)).toString(), barText);

    QVERIFY(history->remove(barUuid));
    QCOMPARE(history->data(history->index(0, 0)).toString(), fooText);

    QVERIFY(history->remove(fooUuid));
    QCOMPARE(history->rowCount(), 0);
}

void HistoryModelTest::testClear()
{
    std::shared_ptr<HistoryModel> history = HistoryModel::self();
    std::unique_ptr<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(history.get()));
    history->setMaxSize(10);
    QCOMPARE(history->rowCount(), 0);

    history->clear();
    QCOMPARE(history->rowCount(), 0);

    const QString barText = QStringLiteral("bar");
    const auto barItem = HistoryItem::create(barText);
    const auto barUuid = barItem->uuid();

    // insert some items
    history->insert(HistoryItem::create(QStringLiteral("foo")));
    history->insert(barItem);
    history->insert(HistoryItem::create(QStringLiteral("foobar")));
    history->moveToTop(barUuid);
    QCOMPARE(history->rowCount(), 3);

    // and clear
    history->clear();
    QCOMPARE(history->rowCount(), 0);
}

void HistoryModelTest::testIndexOf()
{
    std::shared_ptr<HistoryModel> history = HistoryModel::self();
    std::unique_ptr<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(history.get()));
    history->setMaxSize(10);
    QCOMPARE(history->rowCount(), 0);
    QVERIFY(history->indexOf(QByteArrayLiteral("whatever")) < 0);
    QVERIFY(history->indexOf(QByteArray()) < 0);

    const QString fooText = QStringLiteral("foo");
    const auto fooItem = HistoryItem::create(fooText);
    const auto fooUuid = fooItem->uuid();

    // insert some items
    history->insert(fooItem);
    QVERIFY(history->indexOf(QByteArrayLiteral("whatever")) < 0);
    QVERIFY(history->indexOf(QByteArray()) < 0);
    QVERIFY(history->indexOf(fooUuid) >= 0);
    QCOMPARE(history->index(history->indexOf(fooUuid)).data(HistoryModel::UuidRole).toByteArray(), fooUuid);

    history->clear();
    QVERIFY(history->indexOf(fooUuid) < 0);
}

void HistoryModelTest::testType_data()
{
    QTest::addColumn<HistoryItem *>("item");
    QTest::addColumn<HistoryItemType>("expectedType");

    auto item = HistoryItem::create(QStringLiteral("foo"));
    QTest::newRow("text") << item << HistoryItemType::Text;
    item = HistoryItem::create(QImage());
    QTest::newRow("image") << item << HistoryItemType::Image;
    item = HistoryItem::create(QList<QUrl>());
    QTest::newRow("url") << item << HistoryItemType::Url;
}

void HistoryModelTest::testType()
{
    std::shared_ptr<HistoryModel> history = HistoryModel::self();
    std::unique_ptr<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(history.get()));
    history->setMaxSize(10);
    QCOMPARE(history->rowCount(), 0);

    QFETCH(HistoryItemPtr, item);
    QFETCH(HistoryItemType, expectedType);
    history->insert(item);
    QCOMPARE(history->index(0).data(HistoryModel::TypeRole).value<HistoryItemType>(), expectedType);
}

QTEST_MAIN(HistoryModelTest)
#include "historymodeltest.moc"
