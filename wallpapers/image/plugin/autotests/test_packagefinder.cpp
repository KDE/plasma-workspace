/*
    SPDX-FileCopyrightText: 2016 Antonio Larrosa <larrosa@kde.org>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QDir>
#include <QtTest>

#include <KPackage/PackageLoader>

#include "commontestdata.h"
#include "finder/packagefinder.h"

class PackageFinderTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void testFindPreferredSizeInPackage_data();
    void testFindPreferredSizeInPackage();
    void testPackageFinderCanFindPackages();

private:
    QDir m_dataDir;
};

void PackageFinderTest::initTestCase()
{
    m_dataDir = QDir(QFINDTESTDATA("testdata/default"));
    QVERIFY(!m_dataDir.isEmpty());
}

void PackageFinderTest::testFindPreferredSizeInPackage_data()
{
    // The list of possible screen resolutions to test and the appropriate images that should be chosen
    QTest::addColumn<QSize>("resolution");
    QTest::addColumn<QString>("expected");
    QTest::newRow("1280x1024") << QSize(1280, 1024) << QStringLiteral("1280x1024");
    QTest::newRow("1350x1080") << QSize(1350, 1080) << QStringLiteral("1280x1024");
    QTest::newRow("1440x1080") << QSize(1440, 1080) << QStringLiteral("1600x1200");
    QTest::newRow("1600x1200") << QSize(1600, 1200) << QStringLiteral("1600x1200");
    QTest::newRow("1920x1080") << QSize(1920, 1080) << QStringLiteral("1920x1080");
    QTest::newRow("1920x1200") << QSize(1920, 1200) << QStringLiteral("1920x1200");
    QTest::newRow("3840x2400") << QSize(3840, 2400) << QStringLiteral("3200x2000");
    QTest::newRow("4096x2160") << QSize(4096, 2160) << QStringLiteral("3840x2160");
    QTest::newRow("3840x2160") << QSize(3840, 2160) << QStringLiteral("3840x2160");
    QTest::newRow("3200x1800") << QSize(3200, 1800) << QStringLiteral("3200x1800");
    QTest::newRow("2048x1080") << QSize(2048, 1080) << QStringLiteral("1920x1080");
    QTest::newRow("1680x1050") << QSize(1680, 1050) << QStringLiteral("1680x1050");
    QTest::newRow("1400x1050") << QSize(1400, 1050) << QStringLiteral("1600x1200");
    QTest::newRow("1440x900") << QSize(1440, 900) << QStringLiteral("1440x900");
    QTest::newRow("1280x960") << QSize(1280, 960) << QStringLiteral("1600x1200");
    QTest::newRow("1280x854") << QSize(1280, 854) << QStringLiteral("1280x800");
    QTest::newRow("1280x800") << QSize(1280, 800) << QStringLiteral("1280x800");
    QTest::newRow("1280x720") << QSize(1280, 720) << QStringLiteral("1366x768");
    QTest::newRow("1152x768") << QSize(1152, 768) << QStringLiteral("1280x800");
    QTest::newRow("1024x768") << QSize(1024, 768) << QStringLiteral("1024x768");
    QTest::newRow("800x600") << QSize(800, 600) << QStringLiteral("1024x768");
    QTest::newRow("848x480") << QSize(848, 480) << QStringLiteral("1366x768");
    QTest::newRow("720x480") << QSize(720, 480) << QStringLiteral("1280x800");
    QTest::newRow("640x480") << QSize(640, 480) << QStringLiteral("1024x768");
    QTest::newRow("1366x768") << QSize(1366, 768) << QStringLiteral("1366x768");
    QTest::newRow("1600x814") << QSize(1600, 814) << QStringLiteral("1920x1080");
}

void PackageFinderTest::testFindPreferredSizeInPackage()
{
    QFETCH(QSize, resolution);
    QFETCH(QString, expected);

    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));
    package.setPath(m_dataDir.absoluteFilePath(ImageBackendTestData::defaultPackageFolderName2));

    QVERIFY(package.isValid());
    QVERIFY(package.metadata().isValid());

    PackageFinder::findPreferredImageInPackage(package, resolution);

    QVERIFY(package.filePath("preferred").contains(expected));
}

void PackageFinderTest::testPackageFinderCanFindPackages()
{
    PackageFinder *finder =
        new PackageFinder({m_dataDir.absolutePath(), m_dataDir.absoluteFilePath(ImageBackendTestData::defaultPackageFolderName2)}, QSize(1920, 1080));
    QSignalSpy spy(finder, &PackageFinder::packageFound);

    QThreadPool::globalInstance()->start(finder);

    spy.wait(10 * 1000);
    QCOMPARE(spy.size(), 1);

    const auto items = spy.takeFirst().at(0).value<QList<KPackage::Package>>();
    // Total 3 packages in the directory, but one package is broken and should not be added to the list.
    QCOMPARE(items.size(), ImageBackendTestData::defaultPackageCount);

    // Folders are sorted by names
    // FEATURE207976-dark-wallpaper
    QCOMPARE(items.at(0).filePath("preferred"),
             m_dataDir.absoluteFilePath(QStringLiteral("%1/contents/images/1024x768.png").arg(ImageBackendTestData::defaultPackageFolderName1)));
    QCOMPARE(items.at(0).filePath("preferredDark"),
             m_dataDir.absoluteFilePath(QStringLiteral("%1/contents/images_dark/1920x1080.jpg").arg(ImageBackendTestData::defaultPackageFolderName1)));
    // package
    QCOMPARE(items.at(1).filePath("preferred"),
             m_dataDir.absoluteFilePath(QStringLiteral("%1/contents/images/1920x1080.jpg").arg(ImageBackendTestData::defaultPackageFolderName2)));
    QCOMPARE(items.at(1).filePath("preferredDark"), QString());
}

QTEST_MAIN(PackageFinderTest)

#include "test_packagefinder.moc"
