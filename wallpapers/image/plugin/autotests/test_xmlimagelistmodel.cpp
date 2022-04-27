/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QtTest>

#include <KIO/PreviewJob>

#include "../finder/xmlfinder.h"
#include "../model/xmlimagelistmodel.h"

class XmlImageListModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void testXmlImageListModelData();
    void testXmlImageListModelIndexOf();
    void testXmlImageListModelLoad();
    void testXmlImageListModelAddBackground();
    void testXmlImageListModelRemoveBackground();

private:
    QPointer<XmlImageListModel> m_model = nullptr;
    QPointer<QSignalSpy> m_countSpy = nullptr;
    QPointer<QSignalSpy> m_dataSpy = nullptr;

    QDir m_dataDir;
    QDir m_alternateDir;
    QString m_lightPath;
    QString m_darkPath;
    QString m_dummyPath;
    QSize m_targetSize;
};

void XmlImageListModelTest::initTestCase()
{
    qRegisterMetaType<QList<WallpaperItem>>();

    m_dataDir = QDir(QFINDTESTDATA("testdata/default"));
    m_alternateDir = QDir(QFINDTESTDATA("testdata/alternate"));
    QVERIFY(!m_dataDir.isEmpty());
    QVERIFY(!m_alternateDir.isEmpty());

    m_lightPath = m_dataDir.absoluteFilePath(QStringLiteral("xml/.light.png"));
    m_darkPath = m_dataDir.absoluteFilePath(QStringLiteral("xml/.dark.png"));
    m_dummyPath = m_alternateDir.absoluteFilePath(QStringLiteral("xml/dummy.xml"));

    m_targetSize = QSize(1920, 1080);

    QStandardPaths::setTestModeEnabled(true);
}

void XmlImageListModelTest::init()
{
    m_model = new XmlImageListModel(m_targetSize, this);
    m_countSpy = new QSignalSpy(m_model, &XmlImageListModel::countChanged);
    m_dataSpy = new QSignalSpy(m_model, &XmlImageListModel::dataChanged);

    // Test loading data
    m_model->load({m_dataDir.absolutePath()});
    m_countSpy->wait(10 * 1000);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(m_model->rowCount(), 2);
}

void XmlImageListModelTest::cleanup()
{
    m_model->deleteLater();
    m_countSpy->deleteLater();
    m_dataSpy->deleteLater();
}

void XmlImageListModelTest::cleanupTestCase()
{
    const QString standardPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/wallpapers/");

    QDir(standardPath).removeRecursively();
}

void XmlImageListModelTest::testXmlImageListModelData()
{
    QPersistentModelIndex idx = m_model->index(0, 0);
    QVERIFY(idx.isValid());

    QCOMPARE(idx.data(Qt::DisplayRole).toString(), QStringLiteral("Default Background (For test purpose, don't translate!)"));

    QCOMPARE(idx.data(ImageRoles::ScreenshotRole), QVariant()); // Not cached yet
    m_dataSpy->wait();
    QCOMPARE(m_dataSpy->size(), 1);
    QCOMPARE(m_dataSpy->takeFirst().at(2).value<QVector<int>>().at(0), ImageRoles::ScreenshotRole);
    QVERIFY(!idx.data(ImageRoles::ScreenshotRole).value<QPixmap>().isNull());

    QCOMPARE(idx.data(ImageRoles::AuthorRole).toString(), QStringLiteral("KDE Contributor"));

    QCOMPARE(idx.data(ImageRoles::ResolutionRole).toString(), QString());
    m_dataSpy->wait();
    QCOMPARE(m_dataSpy->size(), 1);
    QCOMPARE(m_dataSpy->takeFirst().at(2).value<QVector<int>>().at(0), ImageRoles::ResolutionRole);
    QCOMPARE(idx.data(ImageRoles::ResolutionRole).toString(), QStringLiteral("415x282"));

    QCOMPARE(idx.data(ImageRoles::PathRole).toUrl().toLocalFile(), m_lightPath);
    QVERIFY(idx.data(ImageRoles::PackageNameRole).toString().startsWith(QStringLiteral("image://gnome-wp-list/get?")));

    QCOMPARE(idx.data(ImageRoles::RemovableRole).toBool(), false);
    QCOMPARE(idx.data(ImageRoles::PendingDeletionRole).toBool(), false);
}

void XmlImageListModelTest::testXmlImageListModelIndexOf()
{
    const QString packagePath = m_model->index(0, 0).data(ImageRoles::PackageNameRole).toString();

    QCOMPARE(m_model->indexOf(packagePath), 0);
    QCOMPARE(m_model->indexOf(m_lightPath), -1);
}

void XmlImageListModelTest::testXmlImageListModelLoad()
{
    m_model->load({m_alternateDir.absolutePath()});
    m_countSpy->wait(10 * 1000);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(m_model->rowCount(), 1);
    QTRY_COMPARE(m_model->index(0, 0).data(Qt::DisplayRole).toString(), QStringLiteral("Dummy Background (For test purpose, don't translate!)"));
    QTRY_COMPARE(m_model->index(0, 0).data(ImageRoles::AuthorRole).toString(), QStringLiteral("Dummy"));
}

void XmlImageListModelTest::testXmlImageListModelAddBackground()
{
    // Case 1: add a valid xml file
    QStringList results = m_model->addBackground(m_dummyPath);
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(results.size(), 1);
    QVERIFY(results.at(0).startsWith(QStringLiteral("image://gnome-wp-list/get?")));
    QCOMPARE(m_model->rowCount(), 3);

    QPersistentModelIndex idx = m_model->index(0, 0); // This is the newly added item.
    QVERIFY(idx.isValid());

    QCOMPARE(idx.data(Qt::DisplayRole).toString(), QStringLiteral("Dummy Background (For test purpose, don't translate!)"));
    QCOMPARE(idx.data(ImageRoles::RemovableRole).toBool(), true);

    // Case 2: add an existing xml file
    results = m_model->addBackground(m_dummyPath);
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Case 3: add an image, not xml file
    results = m_model->addBackground(m_alternateDir.absoluteFilePath(QStringLiteral("dummy.jpg")));
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Case 4: path is empty
    results = m_model->addBackground(QString());
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Case 5: add an invalid xml file
    QVERIFY(QFile::exists(m_alternateDir.absoluteFilePath(QStringLiteral("xml/invalid.xml"))));
    results = m_model->addBackground(m_alternateDir.absoluteFilePath(QStringLiteral("xml/invalid.xml")));
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_countSpy->size(), 0);

    // Test PendingDeletionRole
    QVERIFY(m_model->setData(idx, true, ImageRoles::PendingDeletionRole));
    QCOMPARE(m_dataSpy->size(), 1);
    QCOMPARE(m_dataSpy->takeFirst().at(2).value<QVector<int>>().at(0), ImageRoles::PendingDeletionRole);
    QCOMPARE(idx.data(ImageRoles::PendingDeletionRole).toBool(), true);
}

void XmlImageListModelTest::testXmlImageListModelRemoveBackground()
{
    QCOMPARE(m_model->m_removableWallpapers.size(), 0);

    m_model->addBackground(m_dummyPath);
    m_countSpy->clear();
    QCOMPARE(m_model->m_removableWallpapers.size(), 1);

    QStringList results;

    // Case 1: path is empty
    results = m_model->removeBackground(QString());
    QCOMPARE(m_countSpy->size(), 0);
    QCOMPARE(results.size(), 0);
    QCOMPARE(m_model->m_removableWallpapers.size(), 1);

    // Case 2: remove an existing image
    results = m_model->removeBackground(m_model->m_removableWallpapers.at(0));
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(results.size(), 1);
    QCOMPARE(m_model->rowCount(), 2);
    QCOMPARE(m_model->m_removableWallpapers.size(), 0);

    // Case 3: remove by using root file path
    m_model->addBackground(m_dummyPath);
    m_countSpy->clear();
    results = m_model->removeBackground(m_dummyPath);
    QCOMPARE(m_countSpy->size(), 1);
    QCOMPARE(results.size(), 1);
    m_countSpy->clear();
    QCOMPARE(m_model->rowCount(), 2);

    // Case 3: remove by using image file path
    m_model->addBackground(m_dummyPath);
    m_countSpy->clear();
    results = m_model->removeBackground(m_alternateDir.absoluteFilePath(QStringLiteral("xml/.dummy.png")));
    QCOMPARE(m_countSpy->size(), 1);
    m_countSpy->clear();
    QCOMPARE(results.size(), 1);
    QCOMPARE(m_model->rowCount(), 2);
}

QTEST_MAIN(XmlImageListModelTest)

#include "test_xmlimagelistmodel.moc"
