/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QDir>
#include <QtTest>

#include "commontestdata.h"
#include "finder/imagefinder.h"

class ImageFinderTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testImageFinderCanFindImages();

private:
    QDir m_dataDir;
};

void ImageFinderTest::initTestCase()
{
    m_dataDir = QDir(QFINDTESTDATA("testdata/default"));
    QVERIFY(!m_dataDir.isEmpty());
}

void ImageFinderTest::testImageFinderCanFindImages()
{
    ImageFinder *finder = new ImageFinder({m_dataDir.absolutePath(), m_dataDir.absoluteFilePath(QStringLiteral("thispathdoesnotexist.jpg"))});
    QSignalSpy spy(finder, &ImageFinder::imageFound);

    QThreadPool::globalInstance()->start(finder);

    spy.wait(10 * 1000);
    QCOMPARE(spy.count(), 1);

    /**
     * Expected result:
     *
     * - wallpaper.jpg.jpg is found.
     * - "# BUG454692 file name with hash char.png" is found.
     * - symlinkshouldnotbefoundbythefinder.jpg is ignored.
     * - screenshot.png is ignored.
     * - All images in package/contents/images/ are ignored.
     *
     * So the total number of images found by ImageFinder is 2.
     */
    const auto paths = spy.takeFirst().at(0).toStringList();

    qInfo() << "Found images:" << paths;
    QCOMPARE(paths.size(), ImageBackendTestData::defaultImageCount);
    QTRY_COMPARE(paths.count(m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName1)), 1);
    QTRY_COMPARE(paths.count(m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName2)), 1);
    QCOMPARE(paths.count(m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName3)), 1);
    QCOMPARE(paths.count(m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName4)), 1);
    QCOMPARE(paths.count(m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName5)), 1);
    QCOMPARE(paths.count(m_dataDir.absoluteFilePath(ImageBackendTestData::defaultHiddenImageFileName)), 0);
}

QTEST_MAIN(ImageFinderTest)

#include "test_imagefinder.moc"
