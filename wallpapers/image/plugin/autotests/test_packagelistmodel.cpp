/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QtTest>

#include <KIO/CopyJob>
#include <KIO/PreviewJob>

#include "../finder/mediametadatafinder.h"
#include "../model/packagelistmodel.h"
#include "commontestdata.h"

class PackageListModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void testPackageListModelData();
    void testPackageListModelDarkWallpaperPreview();
    void testPackageListModelIndexOf();
    void testPackageListModelLoad();
    void testPackageListModelAddBackground();
    void testPackageListModelRemoveBackground();
    void testPackageListModelRemoveLocalBackground();

private:
    QPointer<PackageListModel> m_model = nullptr;
    QPointer<QSignalSpy> m_countSpy = nullptr;
    QPointer<QSignalSpy> m_dataSpy = nullptr;

    QDir m_dataDir;
    QDir m_alternateDir;
    QStringList m_packagePaths;
    QString m_dummyPackagePath;
    QSize m_targetSize;
};

void PackageListModelTest::initTestCase()
{
    qRegisterMetaType<MediaMetadata>();

    m_dataDir = QDir(QFINDTESTDATA("testdata/default"));
    m_alternateDir = QDir(QFINDTESTDATA("testdata/alternate"));
    QVERIFY(!m_dataDir.isEmpty());
    QVERIFY(!m_alternateDir.isEmpty());

    m_packagePaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultPackageFolderName1);
    m_packagePaths << m_dataDir.absoluteFilePath(ImageBackendTestData::defaultPackageFolderName2);
    m_dummyPackagePath = m_alternateDir.absoluteFilePath(ImageBackendTestData::alternatePackageFolderName1);

    m_targetSize = QSize(1920, 1080);

    QStandardPaths::setTestModeEnabled(true);
}

void PackageListModelTest::init()
{
    m_model = new PackageListModel(m_targetSize, this);
    m_countSpy = new QSignalSpy(m_model, &PackageListModel::countChanged);
    m_dataSpy = new QSignalSpy(m_model, &PackageListModel::dataChanged);

    // Test loading data
    m_model->load({m_dataDir.absolutePath()});
    m_countSpy->wait(10 * 1000);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(m_model->rowCount(), ImageBackendTestData::defaultPackageCount);
}

void PackageListModelTest::cleanup()
{
    m_model->deleteLater();
    m_countSpy->deleteLater();
    m_dataSpy->deleteLater();
}

void PackageListModelTest::cleanupTestCase()
{
    const QString standardPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/wallpapers/");

    QDir(standardPath).removeRecursively();
}

void PackageListModelTest::testPackageListModelData()
{
    QPersistentModelIndex idx = m_model->index(m_model->indexOf(m_packagePaths.at(1)), 0);
    QVERIFY(idx.isValid());

    QCOMPARE(idx.data(Qt::DisplayRole).toString(), QStringLiteral("Honeywave (For test purpose, don't translate!)"));

    QCOMPARE(idx.data(ImageRoles::ScreenshotRole), QVariant()); // Not cached yet
    const QStringList availablePlugins = KIO::PreviewJob::availablePlugins();
    if (availablePlugins.contains(QLatin1String("imagethumbnail"))) {
        m_dataSpy->wait();
        QCOMPARE(m_dataSpy->size(), 1);
        QCOMPARE(m_dataSpy->takeFirst().at(2).value<QVector<int>>().at(0), ImageRoles::ScreenshotRole);
        QVERIFY(!idx.data(ImageRoles::ScreenshotRole).value<QPixmap>().isNull());
    }

    QCOMPARE(idx.data(ImageRoles::AuthorRole).toString(), QStringLiteral("Ken Vermette"));

    QCOMPARE(idx.data(ImageRoles::ResolutionRole).toString(), QString());
    m_dataSpy->wait();
    QCOMPARE(m_dataSpy->size(), 1);
    QCOMPARE(m_dataSpy->takeFirst().at(2).value<QVector<int>>().at(0), ImageRoles::ResolutionRole);
    QCOMPARE(idx.data(ImageRoles::ResolutionRole).toString(), QStringLiteral("1920x1080"));

    QCOMPARE(idx.data(ImageRoles::PathRole).toUrl().toLocalFile(),
             m_packagePaths.at(1) + QDir::separator() + QStringLiteral("contents") + QDir::separator() + QStringLiteral("images") + QDir::separator()
                 + QStringLiteral("1920x1080.jpg"));
    QCOMPARE(idx.data(ImageRoles::PackageNameRole).toString(), m_packagePaths.at(1) + QDir::separator());

    QCOMPARE(idx.data(ImageRoles::RemovableRole).toBool(), false);
    QCOMPARE(idx.data(ImageRoles::PendingDeletionRole).toBool(), false);
}

void PackageListModelTest::testPackageListModelDarkWallpaperPreview()
{
    QModelIndex idx = m_model->index(m_model->indexOf(m_packagePaths.at(0)), 0);
    QVERIFY(idx.isValid());

    QCOMPARE(idx.data(Qt::DisplayRole).toString(), QStringLiteral("Dark Wallpaper (For test purpose, don't translate!)"));

    QCOMPARE(idx.data(ImageRoles::ScreenshotRole), QVariant()); // Not cached yet
    const QStringList availablePlugins = KIO::PreviewJob::availablePlugins();
    if (availablePlugins.contains(QLatin1String("imagethumbnail"))) {
        m_dataSpy->wait();
        QCOMPARE(m_dataSpy->size(), 1);
        QCOMPARE(m_dataSpy->takeFirst().at(2).value<QVector<int>>().at(0), ImageRoles::ScreenshotRole);
        m_dataSpy->clear();

        const auto preview = idx.data(ImageRoles::ScreenshotRole).value<QPixmap>();
        QVERIFY(!preview.isNull());
        const auto previewImage = preview.toImage();
        QCOMPARE(previewImage.pixelColor({0, 0}), Qt::red);
        QCOMPARE(previewImage.pixelColor({previewImage.width() / 2, 0}), Qt::black);
    }
}

void PackageListModelTest::testPackageListModelIndexOf()
{
    QCOMPARE(m_model->indexOf(m_packagePaths.at(0)), 0);
    QCOMPARE(m_model->indexOf(m_packagePaths.at(1)), 1);
    QCOMPARE(m_model->indexOf(QUrl::fromLocalFile(m_packagePaths.at(0)).toString()), 0);
    QCOMPARE(m_model->indexOf(QUrl::fromLocalFile(m_packagePaths.at(1)).toString()), 1);
    QCOMPARE(m_model->indexOf(m_dataDir.absoluteFilePath(QStringLiteral("brokenpackage"))), -1);
}

void PackageListModelTest::testPackageListModelLoad()
{
    m_model->load({m_alternateDir.absolutePath()});
    m_countSpy->wait(10 * 1000);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(m_model->rowCount(), 1);
    QCOMPARE(m_model->index(0, 0).data(Qt::DisplayRole), QStringLiteral("Dummy wallpaper (For test purpose, don't translate!)"));
}

void PackageListModelTest::testPackageListModelAddBackground()
{
    // Case 1: add a valid package
    QStringList results = m_model->addBackground(m_dummyPackagePath);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(results.size(), 1);
    QCOMPARE(results.at(0), m_dummyPackagePath + QDir::separator());
    QCOMPARE(m_model->rowCount(), ImageBackendTestData::defaultPackageCount + 1);

    QPersistentModelIndex idx = m_model->index(0, 0); // This is the newly added item.
    QVERIFY(idx.isValid());

    QCOMPARE(idx.data(Qt::DisplayRole).toString(), QStringLiteral("Dummy wallpaper (For test purpose, don't translate!)"));
    QCOMPARE(idx.data(ImageRoles::RemovableRole).toBool(), true);

    // Case 2: add an existing package
    results = m_model->addBackground(m_dummyPackagePath);
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Case 3: add an image
    results = m_model->addBackground(m_alternateDir.absoluteFilePath(ImageBackendTestData::alternateImageFileName1));
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Case 4: path is empty
    results = m_model->addBackground(QString());
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Case 5: add a broken package
    results = m_model->addBackground(m_dataDir.absoluteFilePath(QStringLiteral("brokenpackage")));
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Test PendingDeletionRole
    QVERIFY(m_model->setData(idx, true, ImageRoles::PendingDeletionRole));
    QCOMPARE(m_dataSpy->size(), 1);
    QCOMPARE(m_dataSpy->takeFirst().at(2).value<QVector<int>>().at(0), ImageRoles::PendingDeletionRole);
    QCOMPARE(idx.data(ImageRoles::PendingDeletionRole).toBool(), true);
}

void PackageListModelTest::testPackageListModelRemoveBackground()
{
    m_model->addBackground(m_dummyPackagePath);
    m_countSpy->clear();
    QCOMPARE(m_model->m_removableWallpapers.size(), 1);
    QCOMPARE(m_model->m_removableWallpapers.at(0), m_dummyPackagePath + QDir::separator());

    QStringList results;

    // Case 1: path is empty
    results = m_model->removeBackground(QString());
    QCOMPARE(m_countSpy->size(), 0);
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_model->m_removableWallpapers.size(), 1);

    // Case 2: remove a not added package
    results = m_model->removeBackground(m_alternateDir.absoluteFilePath(QStringLiteral("thispackagedoesnotexist")));
    QCOMPARE(m_countSpy->size(), 0);
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_model->m_removableWallpapers.size(), 1);

    // Case 3: remove an existing image
    results = m_model->removeBackground(m_dummyPackagePath);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(m_model->rowCount(), ImageBackendTestData::defaultPackageCount);
    QCOMPARE(results.size(), 1);
    QCOMPARE(results.at(0), m_dummyPackagePath + QDir::separator());
    QCOMPARE(m_model->m_removableWallpapers.size(), 0);
}

void PackageListModelTest::testPackageListModelRemoveLocalBackground()
{
    const QString standardPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/wallpapers/");

    QVERIFY(QDir(standardPath).mkpath(standardPath));
    KIO::CopyJob *job = KIO::copy(QUrl::fromLocalFile(m_dummyPackagePath),
                                  QUrl::fromLocalFile(standardPath + QStringLiteral("dummy") + QDir::separator()),
                                  KIO::HideProgressInfo | KIO::Overwrite);
    QVERIFY(job->exec());

    m_model->load({standardPath});
    QVERIFY(m_countSpy->wait(10 * 1000));
    m_countSpy->clear();

    QPersistentModelIndex idx = m_model->index(0, 0);
    QCOMPARE(idx.data(Qt::DisplayRole).toString(), QStringLiteral("Dummy wallpaper (For test purpose, don't translate!)"));
    QCOMPARE(idx.data(ImageRoles::RemovableRole).toBool(), true);

    m_model->removeBackground(standardPath + QStringLiteral("dummy"));
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();

    // The local package should be deleted.
    QVERIFY(!QFileInfo(standardPath + QStringLiteral("dummy")).exists());
    QVERIFY(QDir(standardPath).rmdir(standardPath));
}

QTEST_MAIN(PackageListModelTest)

#include "test_packagelistmodel.moc"
