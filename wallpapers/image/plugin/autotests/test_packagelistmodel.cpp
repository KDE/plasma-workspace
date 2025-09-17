/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

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
    void testPackageListModelIndexOf();
    void testPackageListModelLoad();
    void testPackageListModelAddBackground();
    void testPackageListModelRemoveBackground();
    void testPackageListModelRemoveLocalBackground();

private:
    QPointer<PackageListModel> m_model = nullptr;
    QSignalSpy *m_countSpy = nullptr;
    QSignalSpy *m_dataSpy = nullptr;

    QDir m_dataDir;
    QDir m_alternateDir;
    QList<QUrl> m_packagePaths;
    QUrl m_dummyPackagePath;
    QProperty<QSize> m_targetSize;
    QProperty<bool> m_usedInConfig{true};
};

void PackageListModelTest::initTestCase()
{
    qRegisterMetaType<MediaMetadata>();

    m_dataDir = QDir(QFINDTESTDATA("testdata/default"));
    m_alternateDir = QDir(QFINDTESTDATA("testdata/alternate"));
    QVERIFY(!m_dataDir.isEmpty());
    QVERIFY(!m_alternateDir.isEmpty());

    m_packagePaths << QUrl::fromLocalFile(m_dataDir.absoluteFilePath(ImageBackendTestData::defaultPackageFolderName1));
    m_packagePaths << QUrl::fromLocalFile(m_dataDir.absoluteFilePath(ImageBackendTestData::defaultPackageFolderName2));
    m_dummyPackagePath = QUrl::fromLocalFile(m_alternateDir.absoluteFilePath(ImageBackendTestData::alternatePackageFolderName1));

    m_targetSize = QSize(1920, 1080);

    QStandardPaths::setTestModeEnabled(true);
}

void PackageListModelTest::init()
{
    m_model = new PackageListModel(QBindable<QSize>(&m_targetSize), QBindable<bool>(&m_usedInConfig), this);
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
    delete m_countSpy;
    delete m_dataSpy;
}

void PackageListModelTest::cleanupTestCase()
{
    const QString standardPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/wallpapers/";

    QDir(standardPath).removeRecursively();
}

void PackageListModelTest::testPackageListModelData()
{
    QPersistentModelIndex idx = m_model->index(m_model->indexOf(m_packagePaths.at(1)), 0);
    QVERIFY(idx.isValid());

    QCOMPARE(idx.data(Qt::DisplayRole).toString(), QStringLiteral("Honeywave (For test purpose, don't translate!)"));

    QCOMPARE(idx.data(ImageRoles::AuthorRole).toString(), QStringLiteral("Ken Vermette"));

    QCOMPARE(idx.data(ImageRoles::PathRole).toUrl().toLocalFile(),
             m_packagePaths.at(1).toLocalFile() + QDir::separator() + u"contents" + QDir::separator() + u"images" + QDir::separator() + u"1920x1080.jpg");
    QCOMPARE(idx.data(ImageRoles::PackageNameRole).toString(), m_packagePaths.at(1).toLocalFile() + QDir::separator());

    QCOMPARE(idx.data(ImageRoles::RemovableRole).toBool(), false);
    QCOMPARE(idx.data(ImageRoles::PendingDeletionRole).toBool(), false);
}

void PackageListModelTest::testPackageListModelIndexOf()
{
    QCOMPARE(m_model->indexOf(m_packagePaths.at(0)), 0);
    QCOMPARE(m_model->indexOf(m_packagePaths.at(1)), 1);
    QCOMPARE(m_model->indexOf(QUrl::fromLocalFile(m_dataDir.absoluteFilePath(QStringLiteral("brokenpackage")))), -1);
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
    QCOMPARE(results.at(0), m_dummyPackagePath.toLocalFile() + QDir::separator());
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
    results = m_model->addBackground(QUrl::fromLocalFile(m_alternateDir.absoluteFilePath(ImageBackendTestData::alternateImageFileName1)));
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Case 4: path is empty
    results = m_model->addBackground(QUrl());
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Case 5: add a broken package
    results = m_model->addBackground(QUrl::fromLocalFile(m_dataDir.absoluteFilePath(QStringLiteral("brokenpackage"))));
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Test PendingDeletionRole
    QVERIFY(m_model->setData(idx, true, ImageRoles::PendingDeletionRole));
    QCOMPARE(m_dataSpy->size(), 1);
    QCOMPARE(m_dataSpy->takeFirst().at(2).value<QList<int>>().at(0), ImageRoles::PendingDeletionRole);
    QCOMPARE(idx.data(ImageRoles::PendingDeletionRole).toBool(), true);

    // Case 7: Add a package when usedInConfig: false
    results = m_model->removeBackground(m_dummyPackagePath);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(results.size(), 1);

    m_usedInConfig = false;

    results = m_model->addBackground(m_dummyPackagePath);
    idx = m_model->index(m_model->rowCount() - 1, 0); // This is the newly added item.
    QVERIFY(idx.isValid());
    QCOMPARE(idx.data(Qt::DisplayRole).toString(), QStringLiteral("Dummy wallpaper (For test purpose, don't translate!)"));
    QCOMPARE(idx.data(ImageRoles::RemovableRole).toBool(), true);

    m_usedInConfig = true;
}

void PackageListModelTest::testPackageListModelRemoveBackground()
{
    m_model->addBackground(m_dummyPackagePath);
    m_countSpy->clear();
    QCOMPARE(m_model->m_removableWallpapers.size(), 1);
    QCOMPARE(m_model->m_removableWallpapers.at(0), m_dummyPackagePath.toLocalFile() + QDir::separator());

    QStringList results;

    // Case 1: path is empty
    results = m_model->removeBackground(QUrl());
    QCOMPARE(m_countSpy->size(), 0);
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_model->m_removableWallpapers.size(), 1);

    // Case 2: remove a not added package
    results = m_model->removeBackground(QUrl::fromLocalFile(m_alternateDir.absoluteFilePath(QStringLiteral("thispackagedoesnotexist"))));
    QCOMPARE(m_countSpy->size(), 0);
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_model->m_removableWallpapers.size(), 1);

    // Case 3: remove an existing image
    results = m_model->removeBackground(m_dummyPackagePath);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(m_model->rowCount(), ImageBackendTestData::defaultPackageCount);
    QCOMPARE(results.size(), 1);
    QCOMPARE(results.at(0), m_dummyPackagePath.toLocalFile() + QDir::separator());
    QCOMPARE(m_model->m_removableWallpapers.size(), 0);
}

void PackageListModelTest::testPackageListModelRemoveLocalBackground()
{
    const QString standardPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/wallpapers/";

    QVERIFY(QDir(standardPath).mkpath(standardPath));
    KIO::CopyJob *job = KIO::copy(m_dummyPackagePath, QUrl::fromLocalFile(QString(standardPath + u"dummy" + QDir::separator())), KIO::HideProgressInfo);
    QVERIFY(job->exec());

    m_model->load({standardPath});
    QVERIFY(m_countSpy->wait(10 * 1000));
    m_countSpy->clear();

    QPersistentModelIndex idx = m_model->index(0, 0);
    QCOMPARE(idx.data(Qt::DisplayRole).toString(), QStringLiteral("Dummy wallpaper (For test purpose, don't translate!)"));
    QCOMPARE(idx.data(ImageRoles::RemovableRole).toBool(), true);

    m_model->removeBackground(QUrl::fromLocalFile(QString(standardPath + u"dummy")));
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();

    // The local package should be deleted.
    QVERIFY(!QFileInfo::exists(QString(standardPath + u"dummy")));
    QVERIFY(QDir(standardPath).rmdir(standardPath));
}

QTEST_MAIN(PackageListModelTest)

#include "test_packagelistmodel.moc"
