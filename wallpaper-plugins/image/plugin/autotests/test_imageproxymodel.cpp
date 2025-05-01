/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QDebug>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include <KIO/CopyJob>

#include "../model/imagelistmodel.h"
#include "../model/imageproxymodel.h"
#include "../model/packagelistmodel.h"
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
    QSignalSpy *m_countSpy = nullptr;
    QSignalSpy *m_dataSpy = nullptr;

    QDir m_dataDir;
    QDir m_alternateDir;
    QStringList m_wallpaperPaths;
    QString m_dummyWallpaperPath;
    QStringList m_packagePaths;
    QString m_dummyPackagePath;
    int m_modelNum = 0;
    QProperty<QSize> m_targetSize;
    QProperty<bool> m_usedInConfig{false};
};

void ImageProxyModelTest::initTestCase()
{
    m_dataDir = QDir(QFINDTESTDATA("testdata/default"));
    m_alternateDir = QDir(QFINDTESTDATA("testdata/alternate"));
    QVERIFY(!m_dataDir.isEmpty());
    QVERIFY(!m_alternateDir.isEmpty());
    renameBizarreFile(m_dataDir);

    m_wallpaperPaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName1);
    m_wallpaperPaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName2);
    m_wallpaperPaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName3);
    m_wallpaperPaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName4);
    m_wallpaperPaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName5);
    m_dummyWallpaperPath = m_alternateDir.absoluteFilePath(ImageBackendTestData::alternateImageFileName1);

    m_packagePaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultPackageFolderName1);
    m_packagePaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultPackageFolderName2);
    m_dummyPackagePath = m_alternateDir.absoluteFilePath(ImageBackendTestData::alternatePackageFolderName1);

    m_modelNum = 2;
    m_targetSize = QSize(1920, 1080);

    QStandardPaths::setTestModeEnabled(true);
}

void ImageProxyModelTest::init()
{
    m_model = new ImageProxyModel({m_dataDir.absolutePath()}, QBindable<QSize>(&m_targetSize), QBindable<bool>(&m_usedInConfig), this);
    m_countSpy = new QSignalSpy(m_model, &ImageProxyModel::countChanged);
    m_dataSpy = new QSignalSpy(m_model, &ImageProxyModel::dataChanged);

    QVERIFY(m_model->loading().value());

    // Test loading data
    for (int i = 0; i < m_modelNum; i++) {
        m_countSpy->wait(5 * 1000);

        if (m_countSpy->size() == m_modelNum) {
            break;
        }
    }
    QCOMPARE(m_countSpy->size(), m_modelNum);
    m_countSpy->clear();
    QCOMPARE(m_model->sourceModels().size(), m_modelNum);

    QVERIFY(!m_model->loading().value());

    QCOMPARE(m_model->rowCount(), ImageBackendTestData::defaultTotalCount);
    QCOMPARE(m_model->count(), m_model->rowCount());

    QCOMPARE(m_model->m_imageModel->count(), ImageBackendTestData::defaultImageCount);
    QCOMPARE(m_model->m_packageModel->count(), ImageBackendTestData::defaultPackageCount);
}

void ImageProxyModelTest::cleanup()
{
    m_model->deleteLater();
    delete m_countSpy;
    delete m_dataSpy;
}

void ImageProxyModelTest::cleanupTestCase()
{
    const QString standardPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/wallpapers/");

    QDir(standardPath).removeRecursively();

    restoreBizarreFile(m_dataDir);
}

void ImageProxyModelTest::testImageProxyModelIndexOf()
{
    QVERIFY(m_model->indexOf(m_wallpaperPaths.at(0)) >= 0);
    QVERIFY(m_model->indexOf(m_wallpaperPaths.at(1)) >= 0);
    QVERIFY(m_model->indexOf(m_wallpaperPaths.at(2)) >= 0);
    QVERIFY(m_model->indexOf(m_wallpaperPaths.at(3)) >= 0);
    QVERIFY(m_model->indexOf(m_wallpaperPaths.at(4)) >= 0);
    QVERIFY(m_model->indexOf(m_packagePaths.at(0)) >= 0);
    QVERIFY(m_model->indexOf(m_packagePaths.at(1)) >= 0);
    QVERIFY(m_model->indexOf(m_packagePaths.at(0) + QDir::separator()) >= 0);
    QVERIFY(m_model->indexOf(m_packagePaths.at(1) + QDir::separator()) >= 0);
    QCOMPARE(m_model->indexOf(m_dataDir.absoluteFilePath(u"brokenpackage" + QDir::separator())), -1);
}

void ImageProxyModelTest::testImageProxyModelReload()
{
    m_model->reload();

    for (int i = 0; i < m_modelNum; i++) {
        m_countSpy->wait(5 * 1000);
    }
    QCOMPARE(m_countSpy->size(), m_modelNum);
    m_countSpy->clear();

    QCOMPARE(m_model->rowCount(), ImageBackendTestData::defaultTotalCount);
    QCOMPARE(m_model->m_imageModel->rowCount(), ImageBackendTestData::defaultImageCount);
    QCOMPARE(m_model->m_packageModel->rowCount(), ImageBackendTestData::defaultPackageCount);
}

void ImageProxyModelTest::testImageProxyModelAddBackground()
{
    int count = m_model->count();

    // Case 1: add an existing wallpaper
    auto results = m_model->addBackground(m_wallpaperPaths.at(0));
    QCOMPARE(m_countSpy->size(), 0);
    QCOMPARE(results.size(), 0);

    // Case 2: add an existing package
    results = m_model->addBackground(m_packagePaths.at(0));
    QCOMPARE(results.size(), 0);
    results = m_model->addBackground(m_packagePaths.at(1));
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Case 3: add a new wallpaper
    results = m_model->addBackground(QUrl::fromLocalFile(m_dummyWallpaperPath).toString());
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(results.size(), 1);
    QCOMPARE(m_model->count(), ++count);
    QCOMPARE(m_model->m_imageModel->count(), ImageBackendTestData::defaultImageCount + 1);
    QCOMPARE(m_model->m_packageModel->count(), ImageBackendTestData::defaultPackageCount);

    // Case 4: add a new package
    results = m_model->addBackground(m_dummyPackagePath);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(results.size(), 1);
    QCOMPARE(m_model->count(), ++count);
    QCOMPARE(m_model->m_imageModel->count(), ImageBackendTestData::defaultImageCount + 1);
    QCOMPARE(m_model->m_packageModel->count(), ImageBackendTestData::defaultPackageCount + 1);

    // Test KDirWatch
    QVERIFY(m_model->m_dirWatch.contains(m_dummyWallpaperPath));
    QVERIFY(m_model->m_dirWatch.contains(m_dummyPackagePath));

    QCOMPARE(m_model->m_pendingAddition.size(), 2);
}

void ImageProxyModelTest::testImageProxyModelRemoveBackground()
{
    auto results = m_model->addBackground(m_dummyWallpaperPath);
    QCOMPARE(results.size(), 1);

    results = m_model->addBackground(m_dummyPackagePath);
    QCOMPARE(results.size(), 1);
    QCOMPARE(m_model->m_pendingAddition.size(), 2);
    m_countSpy->clear();

    int count = m_model->count();

    // Case 1: remove an existing wallpaper
    m_model->removeBackground(QUrl::fromLocalFile(m_dummyWallpaperPath).toString());
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(m_model->m_imageModel->count(), ImageBackendTestData::defaultImageCount);
    QCOMPARE(m_model->m_packageModel->count(), ImageBackendTestData::defaultPackageCount + 1);
    QCOMPARE(m_model->count(), --count);
    QVERIFY(!m_model->m_dirWatch.contains(m_dummyWallpaperPath));

    m_model->removeBackground(m_dummyPackagePath);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(m_model->m_imageModel->count(), ImageBackendTestData::defaultImageCount);
    QCOMPARE(m_model->m_packageModel->count(), ImageBackendTestData::defaultPackageCount);
    QCOMPARE(m_model->count(), --count);
    QVERIFY(!m_model->m_dirWatch.contains(m_dummyPackagePath));

    QCOMPARE(m_model->m_pendingAddition.size(), 0);

    // Case 2: remove an unexisting wallpaper
    m_model->removeBackground(m_dummyWallpaperPath);
    QCOMPARE(m_countSpy->size(), 0);
    QCOMPARE(m_model->count(), count);
}

void ImageProxyModelTest::testImageProxyModelDirWatch()
{
    QVERIFY(m_model->m_dirWatch.contains(m_dataDir.absolutePath()));
    // Duplicate watched items as their parent folder is already in KDirWatch
    for (const QString &path : std::as_const(m_wallpaperPaths)) {
        QVERIFY2(!m_model->m_dirWatch.contains(path), qPrintable(path));
    }
    for (const QString &path : std::as_const(m_packagePaths)) {
        QFileInfo info(path);
        while (info.absoluteFilePath() != m_dataDir.absolutePath()) {
            // Because of KDirWatch::WatchSubDirs, some folders will still be added to KDirWatch
            QVERIFY2(m_model->m_dirWatch.contains(info.absoluteFilePath()), qPrintable(info.absoluteFilePath()));
            info = QFileInfo(info.absolutePath()); // Parent folder
        }
    }

    m_model->deleteLater();
    delete m_countSpy;
    delete m_dataSpy;

    // Move to local wallpaper folder
    const QString standardPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/wallpapers/");
    QVERIFY(QDir(standardPath).mkpath(standardPath));

    m_model = new ImageProxyModel({standardPath}, QBindable<QSize>(&m_targetSize), QBindable<bool>(&m_usedInConfig), this);
    m_countSpy = new QSignalSpy(m_model, &ImageProxyModel::countChanged);
    m_dataSpy = new QSignalSpy(m_model, &ImageProxyModel::dataChanged);

    QVERIFY(!m_countSpy->wait(1000));
    QCOMPARE(m_countSpy->size(), 0);
    QCOMPARE(m_model->sourceModels().size(), m_modelNum);
    QCOMPARE(m_model->rowCount(), 0);
    QVERIFY(m_model->m_dirWatch.contains(standardPath));

    // Copy an image to the folder
    QFile imageFile(m_wallpaperPaths.at(0));
    QVERIFY(imageFile.copy(standardPath + QStringLiteral("image.jpg")));
    m_countSpy->wait();
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();

    QCOMPARE(m_model->count(), 1);
    QCOMPARE(m_model->m_imageModel->count(), 1);
    QCOMPARE(m_model->m_packageModel->count(), 0);
    QVERIFY(m_model->m_dirWatch.contains(standardPath));
    // KDirWatch already monitors the parent folder
    QVERIFY(!m_model->m_dirWatch.contains(standardPath + u"image.jpg"));

    // Copy a package to the folder
    auto job = KIO::copy(QUrl::fromLocalFile(m_dummyPackagePath),
                         QUrl::fromLocalFile(QString(standardPath + u"dummy" + QDir::separator())),
                         KIO::HideProgressInfo | KIO::Overwrite);
    job->start();

    if (m_countSpy->empty()) {
        QVERIFY(m_countSpy->wait());
    }
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();

    QCOMPARE(m_model->count(), 2);
    QCOMPARE(m_model->m_imageModel->count(), 1);
    QCOMPARE(m_model->m_packageModel->count(), 1);
    QVERIFY(m_model->m_dirWatch.contains(standardPath));
    // WatchSubDirs
    QVERIFY(m_model->m_dirWatch.contains(standardPath + QStringLiteral("dummy")));

    // Test delete a file
    QFile newImageFile(standardPath + QStringLiteral("image.jpg"));
    QVERIFY(newImageFile.remove());
    m_countSpy->wait();
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();

    QCOMPARE(m_model->count(), 1);
    QCOMPARE(m_model->m_imageModel->count(), 0);
    QCOMPARE(m_model->m_packageModel->count(), 1);
    // Don't remove the file if its parent folder is in KDirWatch, otherwise KDirWatchPrivate::removeEntry will also remove the parent folder
    QVERIFY(m_model->m_dirWatch.contains(standardPath));
    QVERIFY(!m_model->m_dirWatch.contains(standardPath + QStringLiteral("image.jpg")));

    // Test delete a folder
    QVERIFY(QDir(standardPath + QStringLiteral("dummy")).removeRecursively());
    m_countSpy->wait();
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();

    QCOMPARE(m_model->count(), 0);
    QCOMPARE(m_model->m_imageModel->count(), 0);
    QCOMPARE(m_model->m_packageModel->count(), 0);
    QVERIFY(m_model->m_dirWatch.contains(standardPath));
    QVERIFY(!m_model->m_dirWatch.contains(standardPath + QStringLiteral("dummy")));
}

QTEST_MAIN(ImageProxyModelTest)

#include "test_imageproxymodel.moc"
