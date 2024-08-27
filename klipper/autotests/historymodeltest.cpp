/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../historymodel.h"
#include "../historyimageitem.h"
#include "../historystringitem.h"
#include "../historyurlitem.h"
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
    history->insert(std::make_shared<HistoryStringItem>(QStringLiteral("foo")));
    QCOMPARE(history->rowCount(), 0);

    // now it should insert again
    history->setMaxSize(1);
    QCOMPARE(history->maxSize(), 1);
    history->insert(std::make_shared<HistoryStringItem>(QStringLiteral("foo")));
    QCOMPARE(history->rowCount(), 1);

    // insert another item, foo should get removed
    history->insert(std::make_shared<HistoryStringItem>(QStringLiteral("bar")));
    QCOMPARE(history->rowCount(), 1);
    QCOMPARE(history->data(history->index(0, 0)).toString(), QLatin1String("bar"));

    // I don't trust the model, add more items
    history->setMaxSize(10);
    history->insert(std::make_shared<HistoryStringItem>(QStringLiteral("foo")));
    history->insert(std::make_shared<HistoryStringItem>(QStringLiteral("foobar")));
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
    const QString barText = QStringLiteral("bar");
    const QString fooBarText = QStringLiteral("foobar");
    const QByteArray fooUuid = QCryptographicHash::hash(fooText.toUtf8(), QCryptographicHash::Sha1);
    const QByteArray barUuid = QCryptographicHash::hash(barText.toUtf8(), QCryptographicHash::Sha1);
    const QByteArray foobarUuid = QCryptographicHash::hash(fooBarText.toUtf8(), QCryptographicHash::Sha1);

    // let's insert a few items
    history->insert(std::make_shared<HistoryStringItem>(fooText));
    QModelIndex index = history->index(0, 0);
    QCOMPARE(index.data().toString(), fooText);

    history->insert(std::make_shared<HistoryStringItem>(barText));
    QCOMPARE(index.data().toString(), barText);

    history->insert(std::make_shared<HistoryStringItem>(fooBarText));
    QCOMPARE(history->data(history->index(0, 0)).toString(), fooBarText);
    index = history->index(0, 0);

    // insert one again - it should be moved to top
    history->insert(std::make_shared<HistoryStringItem>(barText));
    QCOMPARE(history->data(history->index(0, 0)).toString(), barText);

    // move one to top using the slot
    // already on top, shouldn't change anything
    history->moveToTop(barUuid);
    QCOMPARE(history->data(history->index(0, 0)).toString(), barText);

    // another one should change, though
    history->moveToTop(foobarUuid);
    QCOMPARE(history->data(history->index(0, 0)).toString(), fooBarText);

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

    // insert some items
    history->insert(std::make_shared<HistoryStringItem>(QStringLiteral("foo")));
    history->insert(std::make_shared<HistoryStringItem>(QStringLiteral("bar")));
    history->insert(std::make_shared<HistoryStringItem>(QStringLiteral("foobar")));
    history->moveToTop(QCryptographicHash::hash(QByteArrayLiteral("bar"), QCryptographicHash::Sha1));
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

    // insert some items
    history->insert(std::make_shared<HistoryStringItem>(QStringLiteral("foo")));
    QVERIFY(history->indexOf(QByteArrayLiteral("whatever")) < 0);
    QVERIFY(history->indexOf(QByteArray()) < 0);
    const QByteArray fooUuid = QCryptographicHash::hash(QByteArrayLiteral("foo"), QCryptographicHash::Sha1);
    QVERIFY(history->indexOf(fooUuid) >= 0);
    QCOMPARE(history->index(history->indexOf(fooUuid)).data(HistoryModel::UuidRole).toByteArray(), fooUuid);

    history->clear();
    QVERIFY(history->indexOf(fooUuid) < 0);
}

void HistoryModelTest::testType_data()
{
    QTest::addColumn<HistoryItem *>("item");
    QTest::addColumn<HistoryItemType>("expectedType");

    HistoryItem *item = new HistoryStringItem(QStringLiteral("foo"));
    QTest::newRow("text") << item << HistoryItemType::Text;
    item = new HistoryImageItem(QImage());
    QTest::newRow("image") << item << HistoryItemType::Image;
    item = new HistoryURLItem(QList<QUrl>(), KUrlMimeData::MetaDataMap(), false);
    QTest::newRow("url") << item << HistoryItemType::Url;
}

void HistoryModelTest::testType()
{
    std::shared_ptr<HistoryModel> history = HistoryModel::self();
    std::unique_ptr<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(history.get()));
    history->setMaxSize(10);
    QCOMPARE(history->rowCount(), 0);

    QFETCH(HistoryItem *, item);
    QFETCH(HistoryItemType, expectedType);
    history->insert(std::shared_ptr<HistoryItem>(item));
    QCOMPARE(history->index(0).data(HistoryModel::TypeRole).value<HistoryItemType>(), expectedType);
}

QTEST_MAIN(HistoryModelTest)
#include "historymodeltest.moc"
