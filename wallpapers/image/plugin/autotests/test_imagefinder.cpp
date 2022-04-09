/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QDir>
#include <QtTest>

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
     * - symlinkshouldnotbefoundbythefinder.jpg is ignored.
     * - screenshot.png is ignored.
     * - All images in package/contents/images/ are ignored.
     *
     * So the total number of images found by ImageFinder is 1.
     */
    const auto paths = spy.takeFirst().at(0).toStringList();

    qInfo() << "Found images:" << paths;
    QCOMPARE(paths.size(), 1);
    QCOMPARE(paths.at(0), m_dataDir.absoluteFilePath(QStringLiteral("wallpaper.jpg.jpg")));
}

QTEST_MAIN(ImageFinderTest)

#include "test_imagefinder.moc"
