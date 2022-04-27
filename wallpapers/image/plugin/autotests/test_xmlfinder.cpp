/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QDir>
#include <QtTest>

#include "../finder/xmlfinder.h"

class XmlFinderTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testXmlFinderCanFindImages();
    void testXmlFinderFindPreferredImage();

private:
    QDir m_dataDir;
};

void XmlFinderTest::initTestCase()
{
    qRegisterMetaType<QList<WallpaperItem>>();

    m_dataDir = QDir(QFINDTESTDATA("testdata/default"));
    QVERIFY(!m_dataDir.isEmpty());
}

void XmlFinderTest::testXmlFinderCanFindImages()
{
    XmlFinder *finder = new XmlFinder({m_dataDir.absolutePath()}, QSize(1920, 1080));
    QSignalSpy spy(finder, &XmlFinder::xmlFound);

    QThreadPool::globalInstance()->start(finder);

    spy.wait(10 * 1000);
    QCOMPARE(spy.size(), 1);

    /**
     * Expected result:
     *
     * - 2 wallpapers are found in lightdark.xml, and 2 broken wallpapers are ignored.
     * - .hiddenwallpaper.xml is ignored
     */
    const auto paths = spy.takeFirst().at(0).value<QList<WallpaperItem>>();

    QCOMPARE(paths.size(), 2);

    QTRY_COMPARE(paths.at(0).name, QStringLiteral("Default Background (For test purpose, don't translate!)"));
    QTRY_COMPARE(paths.at(0).filename, m_dataDir.absoluteFilePath(QStringLiteral("xml/.light.png")));
    QTRY_COMPARE(paths.at(0).filename_dark, m_dataDir.absoluteFilePath(QStringLiteral("xml/.dark.png")));
    QTRY_COMPARE(paths.at(0).author, QStringLiteral("KDE Contributor"));

    QTRY_COMPARE(paths.at(1).name, QStringLiteral("Time of Day (For test purpose, don't translate!)"));
    QTRY_COMPARE(paths.at(1).filename, m_dataDir.absoluteFilePath(QStringLiteral("xml/timeofday.xml")));
    QTRY_COMPARE(paths.at(1).slideshow.starttime.date(), QDate(2022, 5, 24));
    QTRY_COMPARE(paths.at(1).slideshow.starttime.time(), QTime(8, 0, 0));
    QCOMPARE(paths.at(1).slideshow.data.size(), 4);

    const auto &static1 = paths.at(1).slideshow.data.at(0);
    QTRY_COMPARE(static1.dataType, 0);
    QTRY_COMPARE(static1.duration, 36000);
    QTRY_COMPARE(static1.file, m_dataDir.absoluteFilePath(QStringLiteral("xml/.light.png")));

    const auto &dynamic1 = paths.at(1).slideshow.data.at(1);
    QTRY_COMPARE(dynamic1.dataType, 1);
    QTRY_COMPARE(dynamic1.duration, 7200);
    QTRY_COMPARE(dynamic1.from, m_dataDir.absoluteFilePath(QStringLiteral("xml/.light.png")));
    QTRY_COMPARE(dynamic1.to, m_dataDir.absoluteFilePath(QStringLiteral("xml/.dark.png")));

    const auto &static2 = paths.at(1).slideshow.data.at(2);
    QTRY_COMPARE(static2.dataType, 0);
    QTRY_COMPARE(static2.duration, 36000);
    QTRY_COMPARE(static2.file, m_dataDir.absoluteFilePath(QStringLiteral("xml/.dark.png")));

    const auto &dynamic2 = paths.at(1).slideshow.data.at(3);
    QTRY_COMPARE(dynamic2.dataType, 1);
    QTRY_COMPARE(dynamic2.duration, 7200);
    QTRY_COMPARE(dynamic2.from, m_dataDir.absoluteFilePath(QStringLiteral("xml/.dark.png")));
    QTRY_COMPARE(dynamic2.to, m_dataDir.absoluteFilePath(QStringLiteral("xml/.light.png")));
}

void XmlFinderTest::testXmlFinderFindPreferredImage()
{
    const QStringList paths{
        QStringLiteral("1280x1024.jpg"),
        QStringLiteral("1600x1200.jpg"),
        QStringLiteral("1920x1080.jpg"),
        QStringLiteral("3840x2160.jpg"),
        QStringLiteral("3840x2400.jpg"),
        QStringLiteral("5120x3200.jpg"),
    };
    QCOMPARE(XmlFinder::findPreferredImage(paths, QSize(3840, 2400)), QStringLiteral("3840x2400.jpg"));
}

QTEST_MAIN(XmlFinderTest)

#include "test_xmlfinder.moc"
