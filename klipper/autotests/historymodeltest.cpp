/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../historymodel.h"
#include "../historyitem.h"
#include "systemclipboard.h"
#include "mimetypes.h"

#include <QAbstractItemModelTester>
#include <QMimeData>
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
    const HistoryItemSharedPtr fooItem = HistoryItem::create(fooText);
    const auto fooUuid = fooItem->uuid();

    const QString barText = QStringLiteral("bar");
    const HistoryItemSharedPtr barItem = HistoryItem::create(barText);
    const auto barUuid = barItem->uuid();

    const QString foobarText = QStringLiteral("foobar");
    const HistoryItemSharedPtr foobarItem = HistoryItem::create(foobarText);
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
    const HistoryItemSharedPtr barItem = HistoryItem::create(barText);
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
    const HistoryItemSharedPtr fooItem = HistoryItem::create(fooText);
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

    auto mimeDataRow = []<typename KF, typename V>(const char *tag, const KF &keyOrFunc, const V &value, HistoryItemType type) {
        QMimeData mimeData;
        if constexpr (std::is_same_v<QString, KF>) {
            mimeData.setData(keyOrFunc, value);
        } else if constexpr (std::is_member_function_pointer_v<KF>) {
            (mimeData.*keyOrFunc)(value);
        }
        // `QTestData &operator<<(QTestData &data, const T &value)` will try to
        // append a pointer to the smart pointer instead of the smart pointer.
        // Because of that, we have to use a raw pointer or else we'll segfault.
        QTest::newRow(tag) << HistoryItem::create(&mimeData).release() << type;
    };
    auto readFile = [](const char *filePath) {
        QFile file(QFINDTESTDATA(filePath));
        file.open(QFile::ReadOnly);
        auto data = file.readAll();
        file.close();
        return data;
    };

    mimeDataRow("mimedata_invalid", nullptr, nullptr, HistoryItemType::Invalid);

    mimeDataRow("mimedata_text_text", &QMimeData::setText, QStringLiteral("foo"), HistoryItemType::Text);
    mimeDataRow("mimedata_text_invalid", &QMimeData::setText, QString{}, HistoryItemType::Invalid);

    mimeDataRow("mimedata_html_text", &QMimeData::setHtml, QStringLiteral("foo"), HistoryItemType::Text);
    mimeDataRow("mimedata_html_invalid", &QMimeData::setHtml, QString{}, HistoryItemType::Invalid);

    mimeDataRow("mimedata_image_image", &QMimeData::setImageData, QImage{1, 1, QImage::Format_ARGB32_Premultiplied}, HistoryItemType::Image);
    mimeDataRow("mimedata_image_invalid", &QMimeData::setImageData, QImage{}, HistoryItemType::Invalid);

    mimeDataRow("mimedata_urls_url", &QMimeData::setUrls, QList<QUrl>{QUrl::fromUserInput(QStringLiteral("url"))}, HistoryItemType::Url);
    mimeDataRow("mimedata_urls_invalid", &QMimeData::setUrls, QList<QUrl>{QUrl{}}, HistoryItemType::Invalid);

    mimeDataRow("mimedata_data_invalid", QStringLiteral("asdf"), "asdf", HistoryItemType::Invalid);

    mimeDataRow("mimedata_markdown_text", Mimetypes::Text::markdown, "foo", HistoryItemType::Text);
    mimeDataRow("mimedata_markdown_invalid", Mimetypes::Text::markdown, "", HistoryItemType::Invalid);

    // Image mimetypes will be tested by an appium test because
    // QMimeData::imageData() does not give a QImage for image mimetypes when
    // the data is created by the application calling QMimeData::imageData().

    QTest::newRow("string_text") << HistoryItem::create(QStringLiteral("foo")).release() << HistoryItemType::Text;
    QTest::newRow("string_invalid") << HistoryItem::create(QString{}).release() << HistoryItemType::Invalid;
}

void HistoryModelTest::testType()
{
    std::shared_ptr<HistoryModel> history = HistoryModel::self();
    std::unique_ptr<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(history.get()));
    history->setMaxSize(10);
    QCOMPARE(history->rowCount(), 0);

    QFETCH(HistoryItem *, item);
    QFETCH(HistoryItemType, expectedType);
    QCOMPARE(item ? item->type() : HistoryItemType::Invalid, expectedType);
    if (item) {
        history->insert(HistoryItemSharedPtr(item));
        QCOMPARE(history->index(0).data(HistoryModel::TypeRole).value<HistoryItemType>(), expectedType);
    }
}

QTEST_MAIN(HistoryModelTest)
#include "historymodeltest.moc"
