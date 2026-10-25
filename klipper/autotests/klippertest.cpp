/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../klipper.h"
#include "../historycycler.h"
#include "../historyitem.h"
#include "../historymodel.h"
#include "../klipperpopup.h"
#include "../systemclipboard.h"
#include "../utils.h"
#include "../mimetypes.h"
#include "testhelpers.h"

#include <KConfigGroup>
#include <KIO/CopyJob>
#include <KSharedConfig>
#include <KSystemClipboard>

#include <QDir>
#include <QMimeData>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::Literals;

namespace TestData
{
static const auto xml = "<?xml version='1.0' encoding='UTF-8'?><item>XML data</item>"_ba;
static const auto json = "{\"array\":[\"JSON\",\"data\"]}"_ba;

inline constexpr std::array uriListStrings{
    QByteArrayView{"file:///foo"},
    QByteArrayView{"file:///bar"},
};
static const QList uriList{QUrl::fromLocalFile("/foo"_L1), QUrl::fromLocalFile("/bar"_L1)};

static const auto html = // Generated from QTextEdit in Qt Designer
    u"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\"><html><head><meta name=\"qrichtext\" content=\"1\" /><meta charset=\"utf-8\" /><style type=\"text/css\">p, li { white-space: pre-wrap; } hr { height: 1px; border-width: 0; } li.unchecked::marker { content: \"\\2610\"; } li.checked::marker { content: \"\\2612\"; }</style></head><body style=\" font-family:'Noto Sans'; font-size:10pt; font-weight:400; font-style:normal;\"><p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Qt Rich Text/HTML 4.0 data</p></body></html>"_s;
static const auto plainText = u"Plain text data"_s;
static const auto kdeSuggestedFileName = "a_suggested_filename"_ba;
}

class KlipperTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    /**
        The mime data from Wayland clipboard is not the exact same image, so two
        image items are listed (first time restarting plasmashell). But after
        images are saved to local, they have the same uuid again. So klipper
        starts to become confused after the second time restarting plasmashell.

        This is caused by QPixmap::from(QImage).toImage() erasing metadata in the
        image, and Wayland clipboard is async, so another identical image is added
        to clipboard.

        This test tries to reproduce the process to trigger the original bug by

        1. Enable image history
        2. Copy faulty image data
        3. Reload Klipper

        @see https://bugs.kde.org/465225
     */
    void testBug465225();

    void testSlotPopupMenuToggle();

    void testMimeTypeOrder();
};

void KlipperTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void KlipperTest::testBug465225()
{
    const QString folderPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QDir::separator() + u"klipper" + QDir::separator();
    QVERIFY(QDir().mkpath(folderPath));
    KIO::CopyJob *job = KIO::copy(QUrl::fromLocalFile(QFINDTESTDATA("./data/bug465225/")), QUrl::fromLocalFile(folderPath), KIO::HideProgressInfo);
    QVERIFY(job->exec());

    const QString fileName = QStringLiteral("klipperrc");
    QScopeGuard cleanup([&folderPath, &fileName] {
        QDir(folderPath).removeRecursively();
        QFile(QStandardPaths::locate(QStandardPaths::GenericConfigLocation, fileName)).remove();
    });

    // Prepare config
    auto klipperConfig = KSharedConfig::openConfig(fileName, KConfig::NoGlobals);
    KConfigGroup config(klipperConfig, QStringLiteral("General"));
    config.writeEntry("SaveImages", true);
    config.writeEntry("MaxClipItems", 30);
    QVERIFY(config.sync());

    auto clipboard = KSystemClipboard::instance();
    // Load the history file which contains faulty image data
    {
        clipboard->clear(QClipboard::Clipboard); // Reset local clipboard
        auto model = HistoryModel::self();
        QCOMPARE(model->rowCount(), 1);
        QCOMPARE(model->first()->type(), HistoryItemType::Image);

        QMimeData *data = new QMimeData;
        data->setText(QDateTime::currentDateTime().toString());
        clipboard->setMimeData(data, QClipboard::Clipboard);
        QCoreApplication::processEvents();
        QTRY_COMPARE(model->rowCount(), 2);
        QCoreApplication::processEvents();

        data = new QMimeData;
        data->setImageData(
            QImage(QFINDTESTDATA("./data/bug465225/data/8f9353dabfdcf9aca5a901cd2c4ae6717cac5adc/"
                                 "8f9353dabfdcf9aca5a901cd2c4ae6717cac5adc")));
        clipboard->setMimeData(data, QClipboard::Clipboard);
        QTRY_COMPARE(model->first()->type(), HistoryItemType::Image);
        QCOMPARE(model->rowCount(), 2);
    }

    // Now load the history file again
    {
        clipboard->clear(QClipboard::Clipboard); // Reset local clipboard
        auto model = HistoryModel::self();
        QCOMPARE(model->rowCount(), 2);
        QCOMPARE(model->first()->type(), HistoryItemType::Image);
    }
    QCoreApplication::processEvents();
}

/**
    Verify that slotPopupMenu() toggles the popup visibility,
    rather than just showing it repeatedly.
 */
void KlipperTest::testSlotPopupMenuToggle()
{
    Klipper klipper;
    auto *popup = klipper.popup();
    QVERIFY(popup);

    // Initially not visible
    QVERIFY(!popup->isVisible());

    // Show the popup
    klipper.slotPopupMenu();
    QVERIFY(popup->isVisible());

    // Toggle the popup visibility
    klipper.slotPopupMenu();
    QVERIFY(!popup->isVisible());
}

void KlipperTest::testMimeTypeOrder()
{
    const QString folderPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QDir::separator() + u"klipper" + QDir::separator();
    QVERIFY(QDir().mkpath(folderPath));

    const QString fileName = QStringLiteral("klipperrc");
    QScopeGuard cleanup([&folderPath, &fileName] {
        QDir(folderPath).removeRecursively();
        QFile(QStandardPaths::locate(QStandardPaths::GenericConfigLocation, fileName)).remove();
    });

    // Prepare config
    auto klipperConfig = KSharedConfig::openConfig(fileName, KConfig::NoGlobals);
    KConfigGroup config(klipperConfig, QStringLiteral("General"));
    config.writeEntry("SaveImages", true);
    config.writeEntry("MaxClipItems", 10);
    QVERIFY(config.sync());

    // Validate PNG data
    QFile pngFile(QFINDTESTDATA("./data/formats/rgba.png"));
    QVERIFY2(pngFile.open(QFile::ReadOnly), qUtf8Printable(pngFile.errorString()));
    auto pngData = pngFile.readAll();
    QVERIFY2(!pngData.isEmpty(), qUtf8Printable(pngFile.errorString()));
    const auto [pngImage, imageReaderError] = imageAndErrorForData(pngData, "png");
    QVERIFY2(!pngImage.isNull(), qUtf8Printable(imageReaderError));

    // auto klipper = Klipper::self();
    auto model = HistoryModel::self();
    model->clear();

    QSignalSpy clipboardChangedSpy(KSystemClipboard::instance(), &KSystemClipboard::changed);

    // Add image item
    HistoryItemConstPtr imageItem;
    {
        QMimeData *data = new QMimeData;
        // Explicitly define some high priority formats first.
        data->setData(MimeTypes::png, pngData);
        // Add generated image formats.
        data->setImageData(pngImage);
        // Add low priority metadata formats.
        data->setData(MimeTypes::kdeSuggestedFileName, TestData::kdeSuggestedFileName);
        model->insert(data);
        clipboardChangedSpy.wait();
        QTRY_COMPARE(model->pendingJobs(), 0);
        QTRY_VERIFY(SystemClipboard::self()->findChildren<KJob *>().isEmpty());
        QCOMPARE(model->rowCount(), 1);
        QCOMPARE(model->first()->type(), HistoryItemType::Image);
        imageItem = model->first();
    }

    // Add URL list item
    HistoryItemConstPtr urlItem;
    {
        QMimeData *data = new QMimeData;
        data->setUrls(TestData::uriList);
        model->insert(data);
        clipboardChangedSpy.wait();
        QTRY_COMPARE(model->pendingJobs(), 0);
        QTRY_VERIFY(SystemClipboard::self()->findChildren<KJob *>().isEmpty());
        QCOMPARE(model->rowCount(), 2);
        QCOMPARE(model->first()->type(), HistoryItemType::Url);
        urlItem = model->first();
    }

    // Add text item
    HistoryItemConstPtr textItem;
    {
        QMimeData *data = new QMimeData;
        // Add various text formats.
        data->setHtml(TestData::html);
        data->setData(MimeTypes::xml, TestData::xml);
        data->setText(TestData::plainText);
        data->setData(MimeTypes::json, TestData::json);
        model->insert(data);
        clipboardChangedSpy.wait();
        QTRY_COMPARE(model->pendingJobs(), 0);
        QTRY_VERIFY(SystemClipboard::self()->findChildren<KJob *>().isEmpty());
        QCOMPARE(model->rowCount(), 3);
        QCOMPARE(model->first()->type(), HistoryItemType::Text);
        textItem = model->first();
    }

    // Verify image item format order
    {
        model->moveToTop(imageItem->uuid());
        clipboardChangedSpy.wait();
        QTRY_COMPARE(model->pendingJobs(), 0);
        QTRY_VERIFY(SystemClipboard::self()->findChildren<KJob *>().isEmpty());
        QCOMPARE(model->first(), imageItem);
        auto data = KSystemClipboard::instance()->mimeData(QClipboard::Clipboard);
        const auto formats = data->formats();
        auto foundIterators = Utils::find(formats, MimeTypes::png, MimeTypes::qtImage, MimeTypes::kdeSuggestedFileName);
        const auto [pngIt, qtImageIt, kdeSuggestedFileNameIt] = foundIterators;
        const auto allFound = !std::ranges::contains(foundIterators, formats.cend());
        const auto formatsString = formats.empty() ? "empty format list" : qUtf8Printable(formats.join(u", "));
        QVERIFY2(allFound, formatsString);
        QCOMPARE(*pngIt, MimeTypes::png);
        QCOMPARE(*qtImageIt, MimeTypes::qtImage);
        QCOMPARE(*kdeSuggestedFileNameIt, MimeTypes::kdeSuggestedFileName);
        // comare relative order
        QVERIFY2(pngIt < qtImageIt, formatsString);
        QVERIFY2(qtImageIt < kdeSuggestedFileNameIt, formatsString);
        // compare data
        QCOMPARE(data->data(MimeTypes::png), pngData);
        QCOMPARE(data->imageData().value<QImage>(), pngImage);
        QCOMPARE(data->data(MimeTypes::kdeSuggestedFileName), TestData::kdeSuggestedFileName);
    }

    // Verify URL list item format order
    {
        model->moveToTop(urlItem->uuid());
        clipboardChangedSpy.wait();
        QTRY_COMPARE(model->pendingJobs(), 0);
        QTRY_VERIFY(SystemClipboard::self()->findChildren<KJob *>().isEmpty());
        QCOMPARE(model->first(), urlItem);
        const auto data = KSystemClipboard::instance()->mimeData(QClipboard::Clipboard);
        const auto formats = data->formats();
        auto foundIterators = Utils::find(formats, MimeTypes::uriList, MimeTypes::plainUtf8Text);
        const auto [uriListIt, plainUtf8TextIt] = foundIterators;
        const auto allFound = !std::ranges::contains(foundIterators, formats.cend());
        const auto formatsString = formats.empty() ? "empty format list" : qUtf8Printable(formats.join(u", "));
        QVERIFY2(allFound, formatsString);
        QCOMPARE(*uriListIt, MimeTypes::uriList);
        QCOMPARE(*plainUtf8TextIt, MimeTypes::plainUtf8Text);
        QVERIFY2(uriListIt < plainUtf8TextIt, formatsString);
        QCOMPARE(data->urls(), TestData::uriList);
        static_assert(std::same_as<std::ranges::const_iterator_t<QByteArray>, const char *>);
        const auto uriListText = data->data(MimeTypes::plainUtf8Text);
        for (const auto expectedUri : TestData::uriListStrings) {
            const auto foundUri = std::ranges::search(uriListText, expectedUri);
            const auto error = std::format("expected {} in {}", expectedUri.data(), uriListText.data());
            QVERIFY2(!foundUri.empty(), error.c_str());
        }
    }

    // Verify text item format order
    {
        model->moveToTop(textItem->uuid());
        clipboardChangedSpy.wait();
        QTRY_COMPARE(model->pendingJobs(), 0);
        QTRY_VERIFY(SystemClipboard::self()->findChildren<KJob *>().isEmpty());
        QCOMPARE(model->first(), textItem);
        const auto data = KSystemClipboard::instance()->mimeData(QClipboard::Clipboard);
        const auto formats = data->formats();
        auto foundIterators = Utils::find(formats, MimeTypes::html, MimeTypes::xml, MimeTypes::plainText, MimeTypes::json);
        const auto [htmlIt, xmlIt, plainTextIt, jsonIt] = foundIterators;
        const auto allFound = !std::ranges::contains(foundIterators, formats.cend());
        const auto formatsString = formats.empty() ? "empty format list" : qUtf8Printable(formats.join(u", "));
        QVERIFY2(allFound, formatsString);
        QCOMPARE(*htmlIt, MimeTypes::html);
        QCOMPARE(*xmlIt, MimeTypes::xml);
        QCOMPARE(*plainTextIt, MimeTypes::plainText);
        QCOMPARE(*jsonIt, MimeTypes::json);
        QVERIFY2(htmlIt < xmlIt, formatsString);
        QVERIFY2(xmlIt < plainTextIt, formatsString);
        QVERIFY2(plainTextIt < jsonIt, formatsString);
        QCOMPARE(data->html(), TestData::html);
        QCOMPARE(data->data(MimeTypes::xml), TestData::xml);
        QCOMPARE(data->text(), TestData::plainText);
        QCOMPARE(data->data(MimeTypes::json), TestData::json);
    }
}

QTEST_MAIN(KlipperTest)
#include "klippertest.moc"
