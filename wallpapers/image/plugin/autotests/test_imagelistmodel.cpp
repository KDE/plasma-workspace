/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QtTest>

#include <KIO/PreviewJob>

#include "../finder/mediametadatafinder.h"
#include "../model/imagelistmodel.h"
#include "commontestdata.h"
#include "config-KF5KExiv2.h"

class ImageListModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void testImageListModelData();
    void testImageListModelIndexOf();
    void testImageListModelLoad();
    void testImageListModelAddBackground();
    void testImageListModelRemoveBackground();
    void testImageListModelRemoveLocalBackground();

private:
    QPointer<ImageListModel> m_model = nullptr;
    QPointer<QSignalSpy> m_countSpy = nullptr;
    QPointer<QSignalSpy> m_dataSpy = nullptr;

    QDir m_dataDir;
    QDir m_alternateDir;
    QStringList m_wallpaperPaths;
    QString m_dummyWallpaperPath;
    QSize m_targetSize;
};

void ImageListModelTest::initTestCase()
{
    qRegisterMetaType<MediaMetadata>();

    m_dataDir = QDir(QFINDTESTDATA("testdata/default"));
    m_alternateDir = QDir(QFINDTESTDATA("testdata/alternate"));
    QVERIFY(!m_dataDir.isEmpty());
    QVERIFY(!m_alternateDir.isEmpty());

    m_wallpaperPaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName1);
    m_wallpaperPaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName2);
    m_wallpaperPaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName3);
    m_wallpaperPaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName4);
    m_wallpaperPaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName5);
    m_dummyWallpaperPath = m_alternateDir.absoluteFilePath(ImageBackendTestData::alternateImageFileName1);

    m_targetSize = QSize(1920, 1080);

    QStandardPaths::setTestModeEnabled(true);
}

void ImageListModelTest::init()
{
    m_model = new ImageListModel(m_targetSize, this);
    m_countSpy = new QSignalSpy(m_model, &ImageListModel::countChanged);
    m_dataSpy = new QSignalSpy(m_model, &ImageListModel::dataChanged);

    // Test loading data
    m_model->load({m_dataDir.absolutePath()});
    m_countSpy->wait(10 * 1000);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(m_model->rowCount(), ImageBackendTestData::defaultImageCount);
}

void ImageListModelTest::cleanup()
{
    m_model->deleteLater();
    m_countSpy->deleteLater();
    m_dataSpy->deleteLater();
}

void ImageListModelTest::cleanupTestCase()
{
    const QString standardPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/wallpapers/");

    QDir(standardPath).removeRecursively();
}

void ImageListModelTest::testImageListModelData()
{
    const int row = m_model->indexOf(m_wallpaperPaths.at(0));
    QPersistentModelIndex idx = m_model->index(row, 0);
    QVERIFY(idx.isValid());

    // Should return the complete base name for wallpaper.jpg.jpg
    QCOMPARE(idx.data(Qt::DisplayRole).toString(), QStringLiteral("wallpaper.jpg"));
    m_dataSpy->wait();
#if HAVE_KF5KExiv2
    m_dataSpy->wait();
    m_dataSpy->wait();
    QCOMPARE(m_dataSpy->size(), 3);
    QCOMPARE(m_dataSpy->takeFirst().at(2).value<QVector<int>>().at(0), Qt::DisplayRole);
    QCOMPARE(idx.data(Qt::DisplayRole).toString(), QStringLiteral("DocumentName"));
    QCOMPARE(m_dataSpy->takeFirst().at(2).value<QVector<int>>().at(0), ImageRoles::AuthorRole);
#endif
    QCOMPARE(m_dataSpy->size(), 1);
    QCOMPARE(m_dataSpy->takeFirst().at(2).value<QVector<int>>().at(0), ImageRoles::ResolutionRole);

    QCOMPARE(idx.data(ImageRoles::ScreenshotRole), QVariant()); // Not cached yet
    const QStringList availablePlugins = KIO::PreviewJob::availablePlugins();
    if (availablePlugins.contains(QLatin1String("imagethumbnail"))) {
        m_dataSpy->wait();
        QCOMPARE(m_dataSpy->size(), 1);
        QCOMPARE(m_dataSpy->takeFirst().at(2).value<QVector<int>>().at(0), ImageRoles::ScreenshotRole);
        QVERIFY(!idx.data(ImageRoles::ScreenshotRole).value<QPixmap>().isNull());
    }

#if HAVE_KF5KExiv2
    QCOMPARE(idx.data(ImageRoles::AuthorRole).toString(), QStringLiteral("KDE Community"));
#else
    QCOMPARE(idx.data(ImageRoles::AuthorRole).toString(), QString());
#endif

    QCOMPARE(idx.data(ImageRoles::ResolutionRole).toString(), QStringLiteral("15x16"));

    QCOMPARE(idx.data(ImageRoles::PathRole).toUrl(), QUrl::fromLocalFile(m_wallpaperPaths.at(0)));
    QCOMPARE(idx.data(ImageRoles::PackageNameRole).toString(), m_wallpaperPaths.at(0));

    QCOMPARE(idx.data(ImageRoles::RemovableRole).toBool(), false);
    QCOMPARE(idx.data(ImageRoles::PendingDeletionRole).toBool(), false);
}

void ImageListModelTest::testImageListModelIndexOf()
{
    QTRY_VERIFY(m_model->indexOf(m_wallpaperPaths.at(0)) >= 0);
    QTRY_VERIFY(m_model->indexOf(m_wallpaperPaths.at(1)) >= 0);
    QTRY_VERIFY(m_model->indexOf(m_wallpaperPaths.at(2)) >= 0);
    QVERIFY(m_model->indexOf(m_wallpaperPaths.at(3)) >= 0);
    QVERIFY(m_model->indexOf(m_wallpaperPaths.at(4)) >= 0);
    QTRY_VERIFY(m_model->indexOf(QUrl::fromLocalFile(m_wallpaperPaths.at(0)).toString()) >= 0);
    QCOMPARE(m_model->indexOf(m_dataDir.absoluteFilePath(QStringLiteral(".wallpaper.jpg"))), -1);
}

void ImageListModelTest::testImageListModelLoad()
{
    m_model->load({m_alternateDir.absolutePath()});
    m_countSpy->wait(10 * 1000);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(m_model->rowCount(), ImageBackendTestData::alternateImageCount);
    QCOMPARE(m_model->index(0, 0).data(Qt::DisplayRole), QStringLiteral("dummy"));
}

void ImageListModelTest::testImageListModelAddBackground()
{
    // Case 1: add a valid image
    QStringList results = m_model->addBackground(m_dummyWallpaperPath);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(results.size(), 1);
    QCOMPARE(results.at(0), m_dummyWallpaperPath);
    QCOMPARE(m_model->rowCount(), ImageBackendTestData::defaultImageCount + 1);

    QPersistentModelIndex idx = m_model->index(0, 0); // This is the newly added item.
    QVERIFY(idx.isValid());

    QCOMPARE(idx.data(Qt::DisplayRole).toString(), QStringLiteral("dummy"));
    QCOMPARE(idx.data(ImageRoles::RemovableRole).toBool(), true);

    // Case 2: add an existing image
    results = m_model->addBackground(m_dummyWallpaperPath);
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Case 3: add an unexsting image
    results = m_model->addBackground(m_alternateDir.absoluteFilePath(QStringLiteral("thisimagedoesnotexist.jpg")));
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Case 4: add a hidden image
    QVERIFY(QFile::exists(m_dataDir.absoluteFilePath(QStringLiteral(".wallpaper.jpg"))));
    results = m_model->addBackground(m_dataDir.absoluteFilePath(QStringLiteral(".wallpaper.jpg")));
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Case 5: path is empty
    results = m_model->addBackground(QString());
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Case 6: add a non-image file
    results = m_model->addBackground(m_alternateDir.absoluteFilePath(QStringLiteral("thisisnotanimage.txt")));
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Test PendingDeletionRole
    QVERIFY(m_model->setData(idx, true, ImageRoles::PendingDeletionRole));
    QCOMPARE(m_dataSpy->size(), 1);
    QCOMPARE(m_dataSpy->takeFirst().at(2).value<QVector<int>>().at(0), ImageRoles::PendingDeletionRole);
    QCOMPARE(idx.data(ImageRoles::PendingDeletionRole).toBool(), true);
}

void ImageListModelTest::testImageListModelRemoveBackground()
{
    m_model->addBackground(m_dummyWallpaperPath);
    m_countSpy->clear();
    QCOMPARE(m_model->m_removableWallpapers.size(), 1);
    QCOMPARE(m_model->m_removableWallpapers.at(0), m_dummyWallpaperPath);

    QStringList results;

    // Case 1: path is empty
    results = m_model->removeBackground(QString());
    QCOMPARE(m_countSpy->size(), 0);
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_model->m_removableWallpapers.size(), 1);

    // Case 2: remove a not added image
    results = m_model->removeBackground(m_alternateDir.absoluteFilePath(QStringLiteral("thisimagedoesnotexist.jpg")));
    QCOMPARE(m_countSpy->size(), 0);
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_model->m_removableWallpapers.size(), 1);

    // Case 3: remove an existing image
    results = m_model->removeBackground(m_dummyWallpaperPath);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(results.size(), 1);
    QCOMPARE(results.at(0), m_dummyWallpaperPath);
    QCOMPARE(m_model->rowCount(), ImageBackendTestData::defaultImageCount);
    QCOMPARE(m_model->m_removableWallpapers.size(), 0);
}

void ImageListModelTest::testImageListModelRemoveLocalBackground()
{
    const QString standardPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/wallpapers/");
    QFile imageFile(m_wallpaperPaths.at(0));

    QVERIFY(QDir(standardPath).mkpath(standardPath));
    QVERIFY(imageFile.copy(standardPath + QStringLiteral("wallpaper.jpg.jpg")));

    m_model->load({standardPath});
    QVERIFY(m_countSpy->wait(10 * 1000));
    m_countSpy->clear();

    QPersistentModelIndex idx = m_model->index(0, 0);
    QCOMPARE(idx.data(Qt::DisplayRole).toString(), QStringLiteral("wallpaper.jpg"));
    m_dataSpy->wait();
#if HAVE_KF5KExiv2
    m_dataSpy->wait();
    m_dataSpy->wait();
#endif
    m_dataSpy->clear();
    QCOMPARE(idx.data(ImageRoles::RemovableRole).toBool(), true);

    m_model->removeBackground(standardPath + QStringLiteral("wallpaper.jpg.jpg"));
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();

    // The local wallpaper should be deleted.
    QVERIFY(!QFile::exists(standardPath + QStringLiteral("wallpaper.jpg.jpg")));
    QVERIFY(QDir(standardPath).rmdir(standardPath));
}

QTEST_MAIN(ImageListModelTest)

#include "test_imagelistmodel.moc"
