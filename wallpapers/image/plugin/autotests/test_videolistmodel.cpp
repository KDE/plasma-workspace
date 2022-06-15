/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QtTest>

#include <KIO/PreviewJob>

#include "../model/videolistmodel.h"
#include "commontestdata.h"

class VideoListModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void testVideoListModelData();
    void testVideoListModelIndexOf();
    void testVideoListModelLoad();
    void testVideoListModelAddBackground();
    void testVideoListModelRemoveBackground();
    void testVideoListModelRemoveLocalBackground();

private:
    QPointer<VideoListModel> m_model = nullptr;
    QPointer<QSignalSpy> m_countSpy = nullptr;
    QPointer<QSignalSpy> m_dataSpy = nullptr;

    QDir m_dataDir;
    QDir m_alternateDir;
    QStringList m_videoPaths;
    QString m_dummyVideoPath;
    QSize m_targetSize;
};

void VideoListModelTest::initTestCase()
{
    m_dataDir = QDir(QFINDTESTDATA("testdata/default"));
    m_alternateDir = QDir(QFINDTESTDATA("testdata/alternate"));
    QVERIFY(!m_dataDir.isEmpty());
    QVERIFY(!m_alternateDir.isEmpty());

    m_videoPaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultVideoFileName1);
    m_videoPaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultVideoFileName2);
    m_videoPaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultVideoFileName3);
    m_dummyVideoPath = m_alternateDir.absoluteFilePath(ImageBackendTestData::alternateVideoFileName1);

    m_targetSize = QSize(1920, 1080);

    QStandardPaths::setTestModeEnabled(true);
}

void VideoListModelTest::init()
{
    m_model = new VideoListModel(m_targetSize, this);
    m_countSpy = new QSignalSpy(m_model, &VideoListModel::countChanged);
    m_dataSpy = new QSignalSpy(m_model, &VideoListModel::dataChanged);

    // Test loading data
    m_model->load({m_dataDir.absolutePath()});
    m_countSpy->wait(10 * 1000);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(m_model->rowCount(), ImageBackendTestData::defaultVideoCount);
}

void VideoListModelTest::cleanup()
{
    m_model->deleteLater();
    m_countSpy->deleteLater();
    m_dataSpy->deleteLater();
}

void VideoListModelTest::cleanupTestCase()
{
    const QString standardPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/wallpapers/");

    QDir(standardPath).removeRecursively();
}

void VideoListModelTest::testVideoListModelData()
{
    const int row = m_model->indexOf(m_videoPaths.at(0));
    QPersistentModelIndex idx = m_model->index(row, 0);
    QVERIFY(idx.isValid());

    QCOMPARE(idx.data(Qt::DisplayRole).toString(), QStringLiteral("video"));
    if (!KIO::PreviewJob::availablePlugins().empty()) {
        m_dataSpy->wait();
        m_dataSpy->wait();
        m_dataSpy->wait();
        QCOMPARE(m_dataSpy->size(), 3);
        QCOMPARE(m_dataSpy->takeFirst().at(2).value<QVector<int>>().at(0), Qt::DisplayRole);
        QCOMPARE(m_dataSpy->takeFirst().at(2).value<QVector<int>>().at(0), ImageRoles::AuthorRole);
        QCOMPARE(m_dataSpy->takeFirst().at(2).value<QVector<int>>().at(0), ImageRoles::ResolutionRole);
        QCOMPARE(idx.data(Qt::DisplayRole).toString(), QStringLiteral("Test Video 1"));
    }

    QCOMPARE(idx.data(ImageRoles::ScreenshotRole), QVariant()); // Not cached yet
    if (!KIO::PreviewJob::availablePlugins().empty()) {
        m_dataSpy->wait();
        QCOMPARE(m_dataSpy->size(), 1);
        QCOMPARE(m_dataSpy->takeFirst().at(2).value<QVector<int>>().at(0), ImageRoles::ScreenshotRole);
        QVERIFY(!idx.data(ImageRoles::ScreenshotRole).value<QPixmap>().isNull());
    }

    if (!KIO::PreviewJob::availablePlugins().empty()) {
        QCOMPARE(idx.data(ImageRoles::AuthorRole).toString(), QString());
        QCOMPARE(idx.data(ImageRoles::ResolutionRole).toString(), QStringLiteral("50x28"));
    }

    QCOMPARE(idx.data(ImageRoles::PathRole).toUrl(), QUrl::fromLocalFile(m_videoPaths.at(0)));
    QCOMPARE(idx.data(ImageRoles::PackageNameRole).toString(), m_videoPaths.at(0));

    QCOMPARE(idx.data(ImageRoles::RemovableRole).toBool(), false);
    QCOMPARE(idx.data(ImageRoles::PendingDeletionRole).toBool(), false);
}

void VideoListModelTest::testVideoListModelIndexOf()
{
    QTRY_VERIFY(m_model->indexOf(m_videoPaths.at(0)) >= 0);
    QTRY_VERIFY(m_model->indexOf(m_videoPaths.at(1)) >= 0);
    QTRY_VERIFY(m_model->indexOf(m_videoPaths.at(2)) >= 0);
    QTRY_VERIFY(m_model->indexOf(QUrl::fromLocalFile(m_videoPaths.at(0)).toString()) >= 0);
    QCOMPARE(m_model->indexOf(m_dataDir.absoluteFilePath(QStringLiteral("ghost.mp4"))), -1);
}

void VideoListModelTest::testVideoListModelLoad()
{
    m_model->load({m_alternateDir.absolutePath()});
    m_countSpy->wait(10 * 1000);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(m_model->rowCount(), ImageBackendTestData::alternateVideoCount);
    QCOMPARE(m_model->index(0, 0).data(Qt::DisplayRole), QStringLiteral("dummy"));
}

void VideoListModelTest::testVideoListModelAddBackground()
{
    // Case 1: add a valid video
    QStringList results = m_model->addBackground(m_dummyVideoPath);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(results.size(), 1);
    QCOMPARE(results.at(0), m_dummyVideoPath);
    QCOMPARE(m_model->rowCount(), ImageBackendTestData::defaultVideoCount + 1);

    QPersistentModelIndex idx = m_model->index(0, 0); // This is the newly added item.
    QVERIFY(idx.isValid());

    QCOMPARE(idx.data(Qt::DisplayRole).toString(), QStringLiteral("dummy"));
    if (!KIO::PreviewJob::availablePlugins().empty()) {
        m_dataSpy->wait();
        m_dataSpy->wait();
        m_dataSpy->wait();
        m_dataSpy->clear();
    }
    QCOMPARE(idx.data(ImageRoles::RemovableRole).toBool(), true);

    // Case 2: add an existing video
    results = m_model->addBackground(m_dummyVideoPath);
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Case 3: add an unexisting video
    results = m_model->addBackground(m_alternateDir.absoluteFilePath(QStringLiteral("ghost.mp4")));
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Case 4: add a hidden video
    QVERIFY(QFile::exists(m_dataDir.absoluteFilePath(QStringLiteral(".video.ogv"))));
    results = m_model->addBackground(m_dataDir.absoluteFilePath(QStringLiteral(".video.ogv")));
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Case 5: path is empty
    results = m_model->addBackground(QString());
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Case 6: add a non-video file
    results = m_model->addBackground(m_alternateDir.absoluteFilePath(ImageBackendTestData::alternateImageFileName1));
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Test PendingDeletionRole
    QVERIFY(m_model->setData(idx, true, ImageRoles::PendingDeletionRole));
    QCOMPARE(m_dataSpy->size(), 1);
    QCOMPARE(m_dataSpy->takeFirst().at(2).value<QVector<int>>().at(0), ImageRoles::PendingDeletionRole);
    QCOMPARE(idx.data(ImageRoles::PendingDeletionRole).toBool(), true);
}

void VideoListModelTest::testVideoListModelRemoveBackground()
{
    m_model->addBackground(m_dummyVideoPath);
    m_countSpy->clear();
    QCOMPARE(m_model->m_removableWallpapers.size(), 1);
    QCOMPARE(m_model->m_removableWallpapers.at(0), m_dummyVideoPath);

    QStringList results;

    // Case 1: path is empty
    results = m_model->removeBackground(QString());
    QCOMPARE(m_countSpy->size(), 0);
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_model->m_removableWallpapers.size(), 1);

    // Case 2: remove a not added video
    results = m_model->removeBackground(m_alternateDir.absoluteFilePath(QStringLiteral("ghost.mp4")));
    QCOMPARE(m_countSpy->size(), 0);
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_model->m_removableWallpapers.size(), 1);

    // Case 3: remove an existing video
    results = m_model->removeBackground(m_dummyVideoPath);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(results.size(), 1);
    QCOMPARE(results.at(0), m_dummyVideoPath);
    QCOMPARE(m_model->rowCount(), ImageBackendTestData::defaultVideoCount);
    QCOMPARE(m_model->m_removableWallpapers.size(), 0);
}

void VideoListModelTest::testVideoListModelRemoveLocalBackground()
{
    const QString standardPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/wallpapers/");
    QFile videoFile(m_videoPaths.at(0));

    QVERIFY(QDir(standardPath).mkpath(standardPath));
    QVERIFY(videoFile.copy(standardPath + QStringLiteral("video.mp4")));

    m_model->load({standardPath});
    QVERIFY(m_countSpy->wait(10 * 1000));
    m_countSpy->clear();

    QPersistentModelIndex idx = m_model->index(0, 0);
    QCOMPARE(idx.data(Qt::DisplayRole).toString(), QStringLiteral("video"));
    if (!KIO::PreviewJob::availablePlugins().empty()) {
        m_dataSpy->wait();
        m_dataSpy->wait();
        m_dataSpy->wait();
        m_dataSpy->clear();
    }
    QCOMPARE(idx.data(ImageRoles::RemovableRole).toBool(), true);

    m_model->removeBackground(standardPath + QStringLiteral("video.mp4"));
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();

    // The local wallpaper should be deleted.
    QVERIFY(!QFile::exists(standardPath + QStringLiteral("video.mp4")));
    QVERIFY(QDir(standardPath).rmdir(standardPath));
}

QTEST_MAIN(VideoListModelTest)

#include "test_videolistmodel.moc"
