/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../historymodel.h"
#include "../historyitem.h"
#include "systemclipboard.h"

#include <QAbstractItemModelTester>
#include <QMimeData>
#include <QStandardPaths>
#include <QTest>
#include <QThreadPool>

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
    history->insert(QStringLiteral("foo"));
    QCOMPARE(history->rowCount(), 0);

    // now it should insert again
    history->setMaxSize(1);
    QCOMPARE(history->maxSize(), 1);
    QVERIFY(history->insert(QStringLiteral("foo")));
    QCOMPARE(history->rowCount(), 1);

    // insert another item, foo should get removed
    history->insert(QStringLiteral("bar"));
    QCOMPARE(history->rowCount(), 1);
    QCOMPARE(history->data(history->index(0, 0)).toString(), QLatin1String("bar"));

    // I don't trust the model, add more items
    history->setMaxSize(10);
    history->insert(QStringLiteral("foo"));
    history->insert(QStringLiteral("foobar"));
    QCOMPARE(history->rowCount(), 3);
    QCOMPARE(history->data(history->index(0, 0)).toString(), QLatin1String("foobar"));
    QCOMPARE(history->data(history->index(1, 0)).toString(), QLatin1String("foo"));
    QCOMPARE(history->data(history->index(2, 0)).toString(), QLatin1String("bar"));

    // setting to 0 should clear again
    history->setMaxSize(0);
    QCOMPARE(history->maxSize(), 0);
    QCOMPARE(history->rowCount(), 0);

    QThreadPool::globalInstance()->waitForDone(30000);
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
    const QString fooUuid = QString::fromLatin1(QCryptographicHash::hash(fooText.toUtf8(), QCryptographicHash::Sha1).toHex());
    const QString barUuid = QString::fromLatin1(QCryptographicHash::hash(barText.toUtf8(), QCryptographicHash::Sha1).toHex());
    const QString foobarUuid = QString::fromLatin1(QCryptographicHash::hash(fooBarText.toUtf8(), QCryptographicHash::Sha1).toHex());

    // let's insert a few items
    history->insert(fooText);
    QModelIndex index = history->index(0, 0);
    QCOMPARE(index.data().toString(), fooText);

    history->insert(barText);
    QCOMPARE(index.data().toString(), barText);

    history->insert(fooBarText);
    QCOMPARE(history->data(history->index(0, 0)).toString(), fooBarText);
    index = history->index(0, 0);

    // insert one again - it should be moved to top
    history->insert(barText);
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

    QThreadPool::globalInstance()->waitForDone(30000);
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
    history->insert(QStringLiteral("foo"));
    history->insert(QStringLiteral("bar"));
    history->insert(QStringLiteral("foobar"));
    history->moveToTop(QString::fromLatin1(QCryptographicHash::hash(QByteArrayLiteral("bar"), QCryptographicHash::Sha1).toHex()));
    QCOMPARE(history->rowCount(), 3);

    // and clear
    history->clear();
    QCOMPARE(history->rowCount(), 0);

    QThreadPool::globalInstance()->waitForDone(30000);
}

void HistoryModelTest::testIndexOf()
{
    std::shared_ptr<HistoryModel> history = HistoryModel::self();
    std::unique_ptr<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(history.get()));
    history->setMaxSize(10);
    QCOMPARE(history->rowCount(), 0);
    QVERIFY(history->indexOf(QStringLiteral("whatever")) < 0);
    QVERIFY(history->indexOf(QString()) < 0);

    // insert some items
    history->insert(QStringLiteral("foo"));
    QVERIFY(history->indexOf(QStringLiteral("whatever")) < 0);
    QVERIFY(history->indexOf(QString()) < 0);
    const QString fooUuid = QString::fromLatin1(QCryptographicHash::hash(QByteArrayLiteral("foo"), QCryptographicHash::Sha1).toHex());
    QVERIFY(history->indexOf(fooUuid) >= 0);
    QCOMPARE(history->index(history->indexOf(fooUuid)).data(HistoryModel::UuidRole).toString(), fooUuid);

    history->clear();
    QVERIFY(history->indexOf(fooUuid) < 0);

    QThreadPool::globalInstance()->waitForDone(30000);
}

void HistoryModelTest::testType_data()
{
    QTest::addColumn<QMimeData *>("item");
    QTest::addColumn<HistoryItemType>("expectedType");

    auto item = new QMimeData;
    item->setText(QStringLiteral("foo"));
    QTest::newRow("text") << item << HistoryItemType::Text;
    item = new QMimeData;
    item->setImageData(QImage());
    QTest::newRow("image") << item << HistoryItemType::Image;
    item = new QMimeData;
    item->setText(QStringLiteral("foo"));
    item->setUrls({QUrl(QStringLiteral("file:///home/"))});
    QTest::newRow("url") << item << HistoryItemType::Url;
}

void HistoryModelTest::testType()
{
    std::shared_ptr<HistoryModel> history = HistoryModel::self();
    QAbstractItemModelTester modelTest(history.get());
    history->setMaxSize(10);
    QCOMPARE(history->rowCount(), 0);

    QFETCH(QMimeData *, item);
    QFETCH(HistoryItemType, expectedType);
    history->insert(item);
    QCOMPARE(history->index(0).data(HistoryModel::TypeRole).value<HistoryItemType>(), expectedType);
    history->clear();

    QThreadPool::globalInstance()->waitForDone(30000);
}

QTEST_MAIN(HistoryModelTest)
#include "historymodeltest.moc"
