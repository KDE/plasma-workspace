/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QDebug>
#include <QtTest>

#include <KIO/CopyJob>

#include "../model/imagelistmodel.h"
#include "../model/imageproxymodel.h"
#include "../model/packagelistmodel.h"
#include "../model/videolistmodel.h"
#include "commontestdata.h"

class ImageProxyModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void testImageProxyModelIndexOf();
    void testImageProxyModelReload();
    void testImageProxyModelAddBackground();
    void testImageProxyModelRemoveBackground();
    void testImageProxyModelDirWatch();

private:
    QPointer<ImageProxyModel> m_model = nullptr;
    QPointer<QSignalSpy> m_countSpy = nullptr;
    QPointer<QSignalSpy> m_dataSpy = nullptr;

    QDir m_dataDir;
    QDir m_alternateDir;
    QStringList m_wallpaperPaths;
    QString m_dummyWallpaperPath;
    QStringList m_packagePaths;
    QString m_dummyPackagePath;
    QStringList m_videoPaths;
    QString m_dummyVideoPath;
    QSize m_targetSize;
};

void ImageProxyModelTest::initTestCase()
{
    m_dataDir = QDir(QFINDTESTDATA("testdata/default"));
    m_alternateDir = QDir(QFINDTESTDATA("testdata/alternate"));
    QVERIFY(!m_dataDir.isEmpty());
    QVERIFY(!m_alternateDir.isEmpty());

    m_wallpaperPaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName1);
    m_wallpaperPaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName2);
    m_dummyWallpaperPath = m_alternateDir.absoluteFilePath(ImageBackendTestData::alternateImageFileName1);

    m_packagePaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultPackageFolderName1);
    m_packagePaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultPackageFolderName2);
    m_dummyPackagePath = m_alternateDir.absoluteFilePath(ImageBackendTestData::alternatePackageFolderName1);

    m_videoPaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultVideoFileName1);
    m_videoPaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultVideoFileName2);
    m_videoPaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultVideoFileName3);
    m_dummyVideoPath = m_alternateDir.absoluteFilePath(ImageBackendTestData::alternateVideoFileName1);

    m_targetSize = QSize(1920, 1080);

    QStandardPaths::setTestModeEnabled(true);
}

void ImageProxyModelTest::init()
{
    m_model = new ImageProxyModel({m_dataDir.absolutePath()}, m_targetSize, this);
    m_countSpy = new QSignalSpy(m_model, &ImageProxyModel::countChanged);
    m_dataSpy = new QSignalSpy(m_model, &ImageProxyModel::dataChanged);

    QVERIFY(m_model->loading());

    // Test loading data
    for (int i = 0; i < s_modelNum; i++) {
        m_countSpy->wait(5 * 1000);

        if (m_countSpy->size() == s_modelNum) {
            break;
        }
    }
    QCOMPARE(m_countSpy->size(), s_modelNum);
    m_countSpy->clear();
    QCOMPARE(m_model->sourceModels().size(), s_modelNum);

    QVERIFY(!m_model->loading());

    QCOMPARE(m_model->rowCount(), ImageBackendTestData::defaultTotalCount);
    QCOMPARE(m_model->count(), m_model->rowCount());

    QCOMPARE(m_model->m_imageModel->count(), ImageBackendTestData::defaultImageCount);
    QCOMPARE(m_model->m_packageModel->count(), ImageBackendTestData::defaultPackageCount);
    QCOMPARE(m_model->m_videoModel->count(), ImageBackendTestData::defaultVideoCount);
}

void ImageProxyModelTest::cleanup()
{
    m_model->deleteLater();
    m_countSpy->deleteLater();
    m_dataSpy->deleteLater();
}

void ImageProxyModelTest::cleanupTestCase()
{
    const QString standardPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/wallpapers/");

    QDir(standardPath).removeRecursively();
}

void ImageProxyModelTest::testImageProxyModelIndexOf()
{
    QVERIFY(m_model->indexOf(m_wallpaperPaths.at(0)) >= 0);
    QVERIFY(m_model->indexOf(m_wallpaperPaths.at(1)) >= 0);
    QVERIFY(m_model->indexOf(m_packagePaths.at(0)) >= 0);
    QVERIFY(m_model->indexOf(m_packagePaths.at(1)) >= 0);
    QVERIFY(m_model->indexOf(m_packagePaths.at(0) + QDir::separator()) >= 0);
    QVERIFY(m_model->indexOf(m_packagePaths.at(1) + QDir::separator()) >= 0);
    QCOMPARE(m_model->indexOf(m_dataDir.absoluteFilePath(QStringLiteral("brokenpackage") + QDir::separator())), -1);

    QVERIFY(m_model->indexOf(m_videoPaths.at(0)) >= 0);
    QVERIFY(m_model->indexOf(m_videoPaths.at(1)) >= 0);
    QVERIFY(m_model->indexOf(m_videoPaths.at(2)) >= 0);
}

void ImageProxyModelTest::testImageProxyModelReload()
{
    m_model->reload();

    for (int i = 0; i < s_modelNum; i++) {
        m_countSpy->wait(5 * 1000);
    }
    QCOMPARE(m_countSpy->size(), s_modelNum);
    m_countSpy->clear();

    QCOMPARE(m_model->rowCount(), ImageBackendTestData::defaultTotalCount);
    QCOMPARE(m_model->m_imageModel->rowCount(), ImageBackendTestData::defaultImageCount);
    QCOMPARE(m_model->m_packageModel->rowCount(), ImageBackendTestData::defaultPackageCount);
    QCOMPARE(m_model->m_videoModel->rowCount(), ImageBackendTestData::defaultVideoCount);
}

void ImageProxyModelTest::testImageProxyModelAddBackground()
{
    int count = m_model->count();

    // Case 1.1: add an existing wallpaper
    auto results = m_model->addBackground(m_wallpaperPaths.at(0));
    QCOMPARE(m_countSpy->size(), 0);
    QCOMPARE(results.size(), 0);

    // Case 1.2: add an existing package
    results = m_model->addBackground(m_packagePaths.at(0));
    QCOMPARE(results.size(), 0);
    results = m_model->addBackground(m_packagePaths.at(1));
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Case 1.3: add an existing video
    results = m_model->addBackground(m_videoPaths.at(0));
    QCOMPARE(results.size(), 0);
    results = m_model->addBackground(m_videoPaths.at(1));
    QCOMPARE(results.size(), 0);
    results = m_model->addBackground(m_videoPaths.at(2));
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Case 2.1: add a new wallpaper
    results = m_model->addBackground(QUrl::fromLocalFile(m_dummyWallpaperPath).toString());
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(results.size(), 1);
    QCOMPARE(m_model->count(), ++count);
    QCOMPARE(m_model->m_imageModel->count(), ImageBackendTestData::defaultImageCount + 1);
    QCOMPARE(m_model->m_packageModel->count(), ImageBackendTestData::defaultPackageCount);
    QCOMPARE(m_model->m_videoModel->count(), ImageBackendTestData::defaultVideoCount);

    // Case 2.2: add a new package
    results = m_model->addBackground(m_dummyPackagePath);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(results.size(), 1);
    QCOMPARE(m_model->count(), ++count);
    QCOMPARE(m_model->m_imageModel->count(), ImageBackendTestData::defaultImageCount + 1);
    QCOMPARE(m_model->m_packageModel->count(), ImageBackendTestData::defaultPackageCount + 1);
    QCOMPARE(m_model->m_videoModel->count(), ImageBackendTestData::defaultVideoCount);

    // Case 2.3: add a new video
    results = m_model->addBackground(QUrl::fromLocalFile(m_dummyVideoPath).toString());
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(results.size(), 1);
    QCOMPARE(m_model->count(), ++count);
    QCOMPARE(m_model->m_imageModel->count(), ImageBackendTestData::defaultImageCount + 1);
    QCOMPARE(m_model->m_packageModel->count(), ImageBackendTestData::defaultPackageCount + 1);
    QCOMPARE(m_model->m_videoModel->count(), ImageBackendTestData::defaultVideoCount + 1);

    // Test KDirWatch
    QVERIFY(m_model->m_dirWatch.contains(m_dummyWallpaperPath));
    QVERIFY(m_model->m_dirWatch.contains(m_dummyPackagePath));
    QVERIFY(m_model->m_dirWatch.contains(m_dummyVideoPath));

    QCOMPARE(m_model->m_pendingAddition.size(), 3);
}

void ImageProxyModelTest::testImageProxyModelRemoveBackground()
{
    auto results = m_model->addBackground(m_dummyWallpaperPath);
    QCOMPARE(results.size(), 1);

    results = m_model->addBackground(m_dummyPackagePath);
    QCOMPARE(results.size(), 1);

    results = m_model->addBackground(m_dummyVideoPath);
    QCOMPARE(results.size(), 1);

    QCOMPARE(m_model->m_pendingAddition.size(), 3);
    m_countSpy->clear();

    int count = m_model->count();

    // Case 1: remove an existing wallpaper
    m_model->removeBackground(QUrl::fromLocalFile(m_dummyWallpaperPath).toString());
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(m_model->m_imageModel->count(), ImageBackendTestData::defaultImageCount);
    QCOMPARE(m_model->m_packageModel->count(), ImageBackendTestData::defaultPackageCount + 1);
    QCOMPARE(m_model->m_videoModel->count(), ImageBackendTestData::defaultVideoCount + 1);
    QCOMPARE(m_model->count(), --count);
    QVERIFY(!m_model->m_dirWatch.contains(m_dummyWallpaperPath));

    m_model->removeBackground(m_dummyPackagePath);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(m_model->m_imageModel->count(), ImageBackendTestData::defaultImageCount);
    QCOMPARE(m_model->m_packageModel->count(), ImageBackendTestData::defaultPackageCount);
    QCOMPARE(m_model->m_videoModel->count(), ImageBackendTestData::defaultVideoCount + 1);
    QCOMPARE(m_model->count(), --count);
    QVERIFY(!m_model->m_dirWatch.contains(m_dummyPackagePath));

    m_model->removeBackground(m_dummyVideoPath);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(m_model->m_imageModel->count(), ImageBackendTestData::defaultImageCount);
    QCOMPARE(m_model->m_packageModel->count(), ImageBackendTestData::defaultPackageCount);
    QCOMPARE(m_model->m_videoModel->count(), ImageBackendTestData::defaultVideoCount);
    QCOMPARE(m_model->count(), --count);
    QVERIFY(!m_model->m_dirWatch.contains(m_dummyVideoPath));

    QCOMPARE(m_model->m_pendingAddition.size(), 0);

    // Case 2: remove an unexisting wallpaper
    m_model->removeBackground(m_dummyWallpaperPath);
    QCOMPARE(m_countSpy->size(), 0);
    QCOMPARE(m_model->count(), count);
}

void ImageProxyModelTest::testImageProxyModelDirWatch()
{
    QVERIFY(m_model->m_dirWatch.contains(m_dataDir.absolutePath()));
    QVERIFY(m_model->m_dirWatch.contains(m_wallpaperPaths.at(0)));
    QVERIFY(m_model->m_dirWatch.contains(m_wallpaperPaths.at(1)));
    QVERIFY(m_model->m_dirWatch.contains(m_packagePaths.at(0)));
    QVERIFY(m_model->m_dirWatch.contains(m_packagePaths.at(1)));

    QVERIFY(m_model->m_dirWatch.contains(m_videoPaths.at(0)));
    QVERIFY(m_model->m_dirWatch.contains(m_videoPaths.at(1)));
    QVERIFY(m_model->m_dirWatch.contains(m_videoPaths.at(2)));

    m_model->deleteLater();
    m_countSpy->deleteLater();
    m_dataSpy->deleteLater();

    // Move to local wallpaper folder
    const QString standardPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/wallpapers/");
    QVERIFY(QDir(standardPath).mkpath(standardPath));

    m_model = new ImageProxyModel({standardPath}, m_targetSize, this);
    m_countSpy = new QSignalSpy(m_model, &ImageProxyModel::countChanged);
    m_dataSpy = new QSignalSpy(m_model, &ImageProxyModel::dataChanged);

    QVERIFY(!m_countSpy->wait(1000));
    QCOMPARE(m_countSpy->size(), 0);
    QCOMPARE(m_model->sourceModels().size(), s_modelNum);
    QCOMPARE(m_model->rowCount(), 0);
    QVERIFY(m_model->m_dirWatch.contains(standardPath));

    // Copy an image to the folder
    QFile imageFile(m_wallpaperPaths.at(0));
    QVERIFY(imageFile.copy(standardPath + QStringLiteral("image.jpg")));
    m_countSpy->wait();
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();

    int expectedCount = 0;
    QCOMPARE(m_model->count(), ++expectedCount);
    QCOMPARE(m_model->m_imageModel->count(), 1);
    QCOMPARE(m_model->m_packageModel->count(), 0);
    QCOMPARE(m_model->m_videoModel->count(), 0);
    QVERIFY(m_model->m_dirWatch.contains(standardPath + QStringLiteral("image.jpg")));

    // Copy a package to the folder
    auto job = KIO::copy(QUrl::fromLocalFile(m_dummyPackagePath),
                         QUrl::fromLocalFile(standardPath + QStringLiteral("dummy") + QDir::separator()),
                         KIO::HideProgressInfo | KIO::Overwrite);
    job->start();

    QVERIFY(m_countSpy->wait());
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();

    QCOMPARE(m_model->count(), ++expectedCount);
    QCOMPARE(m_model->m_imageModel->count(), 1);
    QCOMPARE(m_model->m_packageModel->count(), 1);
    QCOMPARE(m_model->m_videoModel->count(), 0);
    QVERIFY(m_model->m_dirWatch.contains(standardPath + QStringLiteral("dummy")));

    // Copy a video to the folder
    QFile videoFile(m_videoPaths.at(0));
    QVERIFY(videoFile.copy(standardPath + QStringLiteral("video.mp4")));
    m_countSpy->wait();
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();

    QCOMPARE(m_model->count(), ++expectedCount);
    QCOMPARE(m_model->m_imageModel->count(), 1);
    QCOMPARE(m_model->m_packageModel->count(), 1);
    QCOMPARE(m_model->m_videoModel->count(), 1);
    QVERIFY(m_model->m_dirWatch.contains(standardPath + QStringLiteral("video.mp4")));

    // Test delete a file
    QFile newImageFile(standardPath + QStringLiteral("image.jpg"));
    QVERIFY(newImageFile.remove());
    m_countSpy->wait();
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();

    QCOMPARE(m_model->count(), --expectedCount);
    QCOMPARE(m_model->m_imageModel->count(), 0);
    QCOMPARE(m_model->m_packageModel->count(), 1);
    QCOMPARE(m_model->m_videoModel->count(), 1);
    QVERIFY(!m_model->m_dirWatch.contains(standardPath + QStringLiteral("image.jpg")));

    // Test delete a folder
    QVERIFY(QDir(standardPath + QStringLiteral("dummy")).removeRecursively());
    m_countSpy->wait();
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();

    QCOMPARE(m_model->count(), --expectedCount);
    QCOMPARE(m_model->m_imageModel->count(), 0);
    QCOMPARE(m_model->m_packageModel->count(), 0);
    QCOMPARE(m_model->m_videoModel->count(), 1);
    QVERIFY(!m_model->m_dirWatch.contains(standardPath + QStringLiteral("dummy")));

    // Test delete a video
    QFile newVideoFile(standardPath + QStringLiteral("video.mp4"));
    QVERIFY(newVideoFile.remove());
    m_countSpy->wait();
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();

    QCOMPARE(m_model->count(), --expectedCount);
    QCOMPARE(m_model->m_imageModel->count(), 0);
    QCOMPARE(m_model->m_packageModel->count(), 0);
    QCOMPARE(m_model->m_videoModel->count(), 0);
    QVERIFY(!m_model->m_dirWatch.contains(standardPath + QStringLiteral("video.mp4")));
}

QTEST_MAIN(ImageProxyModelTest)

#include "test_imageproxymodel.moc"
