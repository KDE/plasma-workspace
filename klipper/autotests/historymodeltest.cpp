/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../historymodel.h"
#include "../historyitem.h"
#include "systemclipboard.h"
#include "testhelpers.h"

#include <QAbstractItemModelTester>
#include <QMimeData>
#include <QStandardPaths>
#include <QTest>

#include <KConfigGroup>
#include <KCoreConfigSkeleton>
#include <KJob>
#include <KSharedConfig>
#include <KSystemClipboard>

namespace
{
QMimeData *password(const QString &text)
{
    auto *data = new QMimeData;
    data->setText(text);
    data->setData(QStringLiteral("x-kde-passwordManagerHint"), QByteArrayLiteral("secret"));
    return data;
}

bool holdsPasswordText(QClipboard::Mode mode, const QString &password)
{
    const QMimeData *data = KSystemClipboard::instance()->mimeData(mode);
    return data && data->text() == password;
}

QMimeData *plain(const QString &text)
{
    auto *data = new QMimeData;
    data->setText(text);
    return data;
}
}

class HistoryModelTest : public QObject
{
    Q_OBJECT
private:
    static void writeSetting(const QString &key, bool value);

private Q_SLOTS:
    void initTestCase();
    void testSetMaxSize();
    void testInsertRemove();
    void testClear();
    void testIndexOf();
    void testType_data();
    void testType();
    void testKeepClipboardContents();
    void testPasswordIsTrackedPerBuffer();
    void testNewSelectionForgetsOnlyThatBuffer_data();
    void testNewSelectionForgetsOnlyThatBuffer();
};

void HistoryModelTest::writeSetting(const QString &key, bool value)
{
    std::shared_ptr<HistoryModel> history = HistoryModel::self();
    auto settings = history->settings()->sharedConfig();
    KConfigGroup group = settings->group(QStringLiteral("General"));
    group.writeEntry(key, value);
    settings->sync();
    history->settings()->read();
    history->loadSettings();
}

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
    QVERIFY(!history->insert(QStringLiteral("foo")));
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
    QVERIFY(history->insert(QStringLiteral("foo")));
    QVERIFY(history->insert(QStringLiteral("foobar")));
    QCOMPARE(history->rowCount(), 3);
    QCOMPARE(history->data(history->index(0, 0)).toString(), QLatin1String("foobar"));
    QCOMPARE(history->data(history->index(1, 0)).toString(), QLatin1String("foo"));
    QCOMPARE(history->data(history->index(2, 0)).toString(), QLatin1String("bar"));

    // setting to 0 should clear again
    history->setMaxSize(0);
    QCOMPARE(history->maxSize(), 0);
    QCOMPARE(history->rowCount(), 0);
    QTRY_COMPARE(history->pendingJobs(), 0);
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
    const QString fooUuid = uuidOf(fooText);
    const QString barUuid = uuidOf(barText);
    const QString foobarUuid = uuidOf(fooBarText);

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
    QTRY_COMPARE(history->pendingJobs(), 0);
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
    QVERIFY(history->insert(QStringLiteral("foo")));
    QVERIFY(history->insert(QStringLiteral("bar")));
    QVERIFY(history->insert(QStringLiteral("foobar")));
    history->moveToTop(uuidOf(QStringLiteral("bar")));
    QCOMPARE(history->rowCount(), 3);

    // and clear
    history->clear();
    QTRY_COMPARE(history->pendingJobs(), 0);
    QCOMPARE(history->rowCount(), 0);
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
    const QString fooUuid = uuidOf(QStringLiteral("foo"));
    QVERIFY(history->indexOf(fooUuid) >= 0);
    QCOMPARE(history->index(history->indexOf(fooUuid)).data(HistoryModel::UuidRole).toString(), fooUuid);

    history->clear();
    QTRY_COMPARE(history->pendingJobs(), 0);
    QVERIFY(history->indexOf(fooUuid) < 0);
}

void HistoryModelTest::testType_data()
{
    QTest::addColumn<std::shared_ptr<QMimeData>>("item");
    QTest::addColumn<HistoryItemType>("expectedType");

    auto item = std::make_shared<QMimeData>();
    item->setText(QStringLiteral("foo"));
    QTest::newRow("text") << item << HistoryItemType::Text;
    item = std::make_shared<QMimeData>();
    item->setImageData(QImage());
    QTest::newRow("image") << item << HistoryItemType::Image;
    item = std::make_shared<QMimeData>();
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

    QFETCH(std::shared_ptr<QMimeData>, item);
    QFETCH(HistoryItemType, expectedType);
    history->insert(item.get());
    QCOMPARE(history->index(0).data(HistoryModel::TypeRole).value<HistoryItemType>(), expectedType);
    history->clear();
    QTRY_COMPARE(history->pendingJobs(), 0);
}

void HistoryModelTest::testKeepClipboardContents()
{
    auto setKeepClipboardContents = [](bool value) {
        std::shared_ptr<HistoryModel> history = HistoryModel::self();
        auto settings = history->settings()->sharedConfig();
        KConfigGroup group = settings->group(QStringLiteral("General"));
        QVERIFY(group.isValid());
        group.writeEntry(QStringLiteral("KeepClipboardContents"), value);
        settings->sync();
        history->settings()->read();
        history->loadSettings();
    };

    {
        std::shared_ptr<HistoryModel> history = HistoryModel::self();
        setKeepClipboardContents(true);
        QVERIFY(history->insert(QStringLiteral("foo")));
        QCOMPARE(history->rowCount(), 1);
        QCOMPARE(history->pendingJobs(), 1);
        QTRY_COMPARE(history->pendingJobs(), 0);
    }

    {
        qDebug() << "Stage 2";
        std::shared_ptr<HistoryModel> history = HistoryModel::self();
        QCOMPARE(history->rowCount(), 1);

        setKeepClipboardContents(false);
    }

    qDebug() << "Stage 3";
    QDir dataDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/klipper/data");
    QVERIFY(!dataDir.exists());

    {
        std::shared_ptr<HistoryModel> history = HistoryModel::self();
        QCOMPARE(history->rowCount(), 0);
        dataDir.refresh();
        QVERIFY(dataDir.exists());
        QCOMPARE(dataDir.entryList(QDir::NoDotAndDotDot).size(), 0);

        QVERIFY(history->insert(QStringLiteral("foobar")));
        QCOMPARE(history->rowCount(), 1);
        QCOMPARE(history->pendingJobs(), 1);
        QTRY_COMPARE(history->pendingJobs(), 0);
        dataDir.refresh();
        qDebug() << dataDir.entryList();
        QCOMPARE(dataDir.entryList().size(), 3); // QList(".", "..", "8843d7f92416211de9ebb963ff4ce28125932878")

        setKeepClipboardContents(true);
    }

    qDebug() << "Stage 4";
    dataDir.refresh();
    QVERIFY(dataDir.exists());
    QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/klipper").removeRecursively();
}
void HistoryModelTest::testPasswordIsTrackedPerBuffer()
// Test that password being cleared or added to one buffer does not mark it for the other buffer.
// Also test clearing history clears password from both buffers.
{
    std::shared_ptr<HistoryModel> history = HistoryModel::self();
    history->clear();
    history->setMaxSize(10);
    writeSetting(QStringLiteral("SaveSelection"), false);

    KSystemClipboard::instance()->setMimeData(password(QStringLiteral("mySuperSecretPassword!")), QClipboard::Clipboard);
    QTRY_VERIFY(history->clipboardHasPassword());
    QTRY_VERIFY(!history->selectionHasPassword());
    KSystemClipboard::instance()->setMimeData(password(QStringLiteral("mySuperSecretPassword!")), QClipboard::Selection);
    QTRY_VERIFY(history->clipboardHasPassword());
    QTRY_VERIFY(history->selectionHasPassword());

    KSystemClipboard::instance()->clear(QClipboard::Selection);
    QTRY_VERIFY(!history->selectionHasPassword());
    QVERIFY(history->clipboardHasPassword());
    QVERIFY(!holdsPasswordText(QClipboard::Selection, QStringLiteral("mySuperSecretPassword!")));

    KSystemClipboard::instance()->clear(QClipboard::Clipboard);
    QTRY_VERIFY(!history->clipboardHasPassword());
    QTRY_VERIFY(!history->selectionHasPassword());
    QVERIFY(!holdsPasswordText(QClipboard::Clipboard, QStringLiteral("mySuperSecretPassword!")));
    QVERIFY(!holdsPasswordText(QClipboard::Selection, QStringLiteral("mySuperSecretPassword!")));

    KSystemClipboard::instance()->setMimeData(password(QStringLiteral("mySuperSecretPassword!")), QClipboard::Clipboard);
    KSystemClipboard::instance()->setMimeData(password(QStringLiteral("mySuperSecretPassword!")), QClipboard::Selection);
    QTRY_VERIFY(history->clipboardHasPassword());
    QTRY_VERIFY(history->selectionHasPassword());
    history->clear();
    QVERIFY(!history->selectionHasPassword());
    QVERIFY(!history->clipboardHasPassword());
    QVERIFY(!holdsPasswordText(QClipboard::Clipboard, QStringLiteral("mySuperSecretPassword!")));
    QVERIFY(!holdsPasswordText(QClipboard::Selection, QStringLiteral("mySuperSecretPassword!")));

    QTRY_COMPARE(history->pendingJobs(), 0);
}

void HistoryModelTest::testNewSelectionForgetsOnlyThatBuffer_data()
{
    QTest::addColumn<bool>("saveSelection");

    QTest::newRow("saved") << true;
    QTest::newRow("unsaved") << false;
}

void HistoryModelTest::testNewSelectionForgetsOnlyThatBuffer()
// Selecting something replaces whatever the selection held, so test if password is gone from it
// both when we don't save selection and when we do.
{
    QFETCH(bool, saveSelection);

    std::shared_ptr<HistoryModel> history = HistoryModel::self();
    history->clear();
    history->setMaxSize(10);
    writeSetting(QStringLiteral("SaveSelection"), saveSelection);

    KSystemClipboard::instance()->setMimeData(password(QStringLiteral("mySuperSecretPassword!")), QClipboard::Clipboard);
    KSystemClipboard::instance()->setMimeData(password(QStringLiteral("mySuperSecretPassword!")), QClipboard::Selection);
    QTRY_VERIFY(history->clipboardHasPassword());
    QTRY_VERIFY(history->selectionHasPassword());
    QCOMPARE(history->rowCount(), 0);

    KSystemClipboard::instance()->setMimeData(plain(QStringLiteral("something new")), QClipboard::Selection);

    QTRY_VERIFY(!history->selectionHasPassword());
    QVERIFY(!holdsPasswordText(QClipboard::Selection, QStringLiteral("mySuperSecretPassword!")));

    QVERIFY(history->clipboardHasPassword());
    QVERIFY(holdsPasswordText(QClipboard::Clipboard, QStringLiteral("mySuperSecretPassword!")));

    // Tests whether selection was saved into history when configured so
    QCOMPARE(history->rowCount(), saveSelection ? 1 : 0);
}

QTEST_MAIN(HistoryModelTest)
#include "historymodeltest.moc"
