/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../klipper.h"
#include "../historycycler.h"
#include "../historyitem.h"
#include "../historymodel.h"
#include "../systemclipboard.h"

#include <KConfigGroup>
#include <KIO/CopyJob>
#include <KSharedConfig>
#include <KSystemClipboard>

#include <QDir>
#include <QMimeData>
#include <QScopeGuard>
#include <QTest>

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
    config.writeEntry("IgnoreImages", false);
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
}

QTEST_MAIN(KlipperTest)
#include "klippertest.moc"
