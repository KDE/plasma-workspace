/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QDir>
#include <QtTest>

#include <KPackage/PackageLoader>

#include "commontestdata.h"
#include "finder/imagepackage.h"

class ImagePackageTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();

    void testLoadStaticPackage();
    void testLoadDarkPackage();
    void testLoadTimedDynamicPackage();
    void testLoadInvalidPackage();

    // Tests for individual functions
    void testParseStartDateTime();
    void testParsePreferredImage();

private:
    QDir m_dataDir;
    KPackage::Package m_package;
};

void ImagePackageTest::initTestCase()
{
    m_dataDir = QDir(QFINDTESTDATA("testdata/default"));
    QVERIFY(!m_dataDir.isEmpty());
}

void ImagePackageTest::init()
{
    m_package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));
}

void ImagePackageTest::testLoadStaticPackage()
{
    m_package.setPath(m_dataDir.absoluteFilePath(ImageBackendTestData::defaultPackageFolderName3));
    QVERIFY(m_package.isValid());

    auto package = KPackage::ImagePackage(m_package, QSize(1920, 1080));
    QVERIFY(package.isValid());
    QCOMPARE(package.dynamicType(), DynamicType::None);

    QVERIFY(package.preferred().toString().contains(QLatin1String("contents/images/1920x1080.jpg")));
    QVERIFY(package.preferredDark().isEmpty());
    QCOMPARE(package.dynamicMetadataSize(), 0);

    auto pair = package.indexAndIntervalAtDateTime(QDateTime::currentDateTime());
    QCOMPARE(pair.first, -1);
    QCOMPARE(pair.second, std::numeric_limits<int>::max());
}

void ImagePackageTest::testLoadDarkPackage()
{
    m_package.setPath(m_dataDir.absoluteFilePath(ImageBackendTestData::defaultPackageFolderName2));
    QVERIFY(m_package.isValid());

    auto package = KPackage::ImagePackage(m_package, QSize(1920, 1080));
    QVERIFY(package.isValid());
    QCOMPARE(package.dynamicType(), DynamicType::None);

    QVERIFY(package.preferred().toString().contains(QLatin1String("contents/images/1024x768.png")));
    QVERIFY(package.preferredDark().toString().contains(QLatin1String("contents/images_dark/1920x1080.jpg")));
    QCOMPARE(package.dynamicMetadataSize(), 0);

    auto pair = package.indexAndIntervalAtDateTime(QDateTime::currentDateTime());
    QCOMPARE(pair.first, -1);
    QCOMPARE(pair.second, std::numeric_limits<int>::max());
}

void ImagePackageTest::testLoadTimedDynamicPackage()
{
    m_package.setPath(m_dataDir.absoluteFilePath(ImageBackendTestData::defaultPackageFolderName1));
    QVERIFY(m_package.isValid());

    auto package = KPackage::ImagePackage(m_package, QSize(1920, 1080));
    QVERIFY(package.isValid());
    QCOMPARE(package.dynamicType(), DynamicType::Timed);

    QVERIFY(package.preferred().isEmpty());
    QVERIFY(package.preferredDark().isEmpty());
    QCOMPARE(package.dynamicMetadataSize(), 4);

    QCOMPARE(package.startTime(), QDateTime(QDate(2022, 5, 24), QTime(8, 0, 0)));

    auto pair = package.indexAndIntervalAtDateTime(QDateTime(QDate(2022, 5, 24), QTime(8, 0, 0)));
    QCOMPARE(pair.first, 0);
    QCOMPARE(pair.second, 36000 * 1000);
    const auto &item0 = package.dynamicMetadataAtIndex(pair.first);
    QCOMPARE(item0.type, DynamicMetadataItem::Static);
    QVERIFY(QFile::exists(item0.filename));
    QVERIFY(item0.filename.contains(QLatin1String("contents/images/day-1024x768.png")));

    pair = package.indexAndIntervalAtDateTime(QDateTime(QDate(2022, 5, 24), QTime(17, 59, 30)));
    QCOMPARE(pair.first, 0);
    QCOMPARE(pair.second, 60 * 1000); // At least 1min
    const auto &item1 = package.dynamicMetadataAtIndex(pair.first);
    QCOMPARE(item1.type, DynamicMetadataItem::Static);
    QVERIFY(QFile::exists(item1.filename));
    QVERIFY(item1.filename.contains(QLatin1String("contents/images/day-1024x768.png")));

    pair = package.indexAndIntervalAtDateTime(QDateTime(QDate(2022, 5, 24), QTime(18, 0, 0)));
    QCOMPARE(pair.first, 1);
    QCOMPARE(pair.second, 600 * 1000);
    const auto &item2 = package.dynamicMetadataAtIndex(pair.first);
    QCOMPARE(item2.type, DynamicMetadataItem::Transition);
    QVERIFY(QFile::exists(item2.filename));
    QVERIFY(item2.filename.contains(QLatin1String("contents/images_dark/night-1920x1080.png")));

    pair = package.indexAndIntervalAtDateTime(QDateTime(QDate(2022, 5, 24), QTime(19, 0, 0)));
    QCOMPARE(pair.first, 1);
    QCOMPARE(pair.second, 600 * 1000);
    const auto &item3 = package.dynamicMetadataAtIndex(pair.first);
    QCOMPARE(item3.type, DynamicMetadataItem::Transition);
    QVERIFY(QFile::exists(item3.filename));
    QVERIFY(item3.filename.contains(QLatin1String("contents/images_dark/night-1920x1080.png")));

    pair = package.indexAndIntervalAtDateTime(QDateTime(QDate(2022, 5, 24), QTime(20, 0, 0)));
    QCOMPARE(pair.first, 2);
    QCOMPARE(pair.second, 36000 * 1000);
    const auto &item4 = package.dynamicMetadataAtIndex(pair.first);
    QCOMPARE(item4.type, DynamicMetadataItem::Static);
    QVERIFY(QFile::exists(item4.filename));
    QVERIFY(item4.filename.contains(QLatin1String("contents/images_dark/night-1920x1080.png")));

    pair = package.indexAndIntervalAtDateTime(QDateTime(QDate(2022, 5, 25), QTime(7, 0, 0)));
    QCOMPARE(pair.first, 3);
    QCOMPARE(pair.second, 600 * 1000);
    const auto &item5 = package.dynamicMetadataAtIndex(pair.first);
    QCOMPARE(item5.type, DynamicMetadataItem::Transition);
    QVERIFY(QFile::exists(item5.filename));
    QVERIFY(item5.filename.contains(QLatin1String("contents/images/day-1024x768.png")));

    pair = package.indexAndIntervalAtDateTime(QDateTime(QDate(2022, 5, 25), QTime(7, 59, 30)));
    QCOMPARE(pair.first, 3);
    QCOMPARE(pair.second, 30 * 1000);
    const auto &item6 = package.dynamicMetadataAtIndex(pair.first);
    QCOMPARE(item6.type, DynamicMetadataItem::Transition);
    QVERIFY(QFile::exists(item6.filename));
    QVERIFY(item6.filename.contains(QLatin1String("contents/images/day-1024x768.png")));
}

void ImagePackageTest::testLoadInvalidPackage()
{
    m_package.setPath(m_dataDir.absoluteFilePath(QStringLiteral("brokenpackage")));
    QVERIFY(m_package.isValid());

    auto package = KPackage::ImagePackage(m_package, QSize(1920, 1080));
    QVERIFY(!package.isValid());
}

void ImagePackageTest::testParseStartDateTime()
{
    auto dataDir = QDir(QFINDTESTDATA("testdata/dynamicmetadata"));
    QVERIFY(!dataDir.isEmpty());

    auto readWallpaperObject = [&dataDir](const QString &filename) {
        QFile file(dataDir.absoluteFilePath(filename));
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        QJsonObject obj = doc.object();
        QJsonValue value = obj.value(QLatin1String("X-KDE-PlasmaImageWallpaper-Dynamic"));
        return value.toObject();
    };

    auto testDateTime1 = KPackage::parseStartDateTime(readWallpaperObject(QStringLiteral("timed1.json")));
    QCOMPARE(testDateTime1, QDateTime(QDate::currentDate().addDays(-1), QTime(8, 0, 0)));

    auto testDateTime2 = KPackage::parseStartDateTime(readWallpaperObject(QStringLiteral("timed2.json")));
    QCOMPARE(testDateTime2, QDateTime(QDate(1, 1, 1), QTime(0, 0, 0)));

    auto testDateTime3 = KPackage::parseStartDateTime(readWallpaperObject(QStringLiteral("timed3.json")));
    QCOMPARE(testDateTime3, QDateTime(QDate(2000, 12, 31), QTime(23, 59, 59)));

    // String to datetime
    auto testDateTime4 = KPackage::parseStartDateTime(readWallpaperObject(QStringLiteral("timed4.json")));
    QCOMPARE(testDateTime4, QDateTime(QDate(2022, 1, 31), QTime(1, 0, 0)));

    auto testDateTime5 = KPackage::parseStartDateTime(readWallpaperObject(QStringLiteral("timed5.json")));
    QCOMPARE(testDateTime5, QDateTime(QDate(2022, 1, 31), QTime(2, 0, 0)));

    auto testDateTime6 = KPackage::parseStartDateTime(readWallpaperObject(QStringLiteral("timed6.json")));
    QCOMPARE(testDateTime6, QDate::currentDate().addDays(-1).startOfDay());
}

void ImagePackageTest::testParsePreferredImage()
{
    const QString absolutePackagePath = m_dataDir.absoluteFilePath(ImageBackendTestData::defaultPackageFolderName3);
    const QString preferred =
        KPackage::parsePreferredImage(absolutePackagePath + QDir::separator() + QStringLiteral("contents%1images%1${resolution}.jpg").arg(QDir::separator()),
                                      QSize(1920, 1080));

    QVERIFY2(preferred.contains(QLatin1String("1920x1080.jpg")), preferred.toLatin1());
}

QTEST_MAIN(ImagePackageTest)

#include "test_imagepackage.moc"
