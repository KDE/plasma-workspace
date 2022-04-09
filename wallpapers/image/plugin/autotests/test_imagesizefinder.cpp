/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QDir>
#include <QtTest>

#include "finder/imagesizefinder.h"

class ImageSizeFinderTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testImageSizeFinder();

private:
    QDir m_dataDir;
};

void ImageSizeFinderTest::initTestCase()
{
    m_dataDir = QDir(QFINDTESTDATA("testdata/default"));
    QVERIFY(!m_dataDir.isEmpty());
}

void ImageSizeFinderTest::testImageSizeFinder()
{
    const QString path = m_dataDir.absoluteFilePath(QStringLiteral("wallpaper.jpg.jpg"));

    ImageSizeFinder *finder = new ImageSizeFinder(path);
    QSignalSpy spy(finder, &ImageSizeFinder::sizeFound);

    QThreadPool::globalInstance()->start(finder);

    spy.wait(10 * 1000);
    QCOMPARE(spy.size(), 1);

    const auto firstSignalResult = spy.takeFirst();
    QCOMPARE(firstSignalResult.size(), 2);

    QCOMPARE(firstSignalResult.at(0).toString(), path);
    QCOMPARE(firstSignalResult.at(1).toSize(), QSize(15, 16));
}

QTEST_MAIN(ImageSizeFinderTest)

#include "test_imagesizefinder.moc"
