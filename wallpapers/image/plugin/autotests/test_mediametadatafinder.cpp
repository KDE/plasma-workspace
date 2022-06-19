/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QDir>
#include <QtTest>

#include "../finder/mediametadatafinder.h"
#include "commontestdata.h"
#include "config-KF5KExiv2.h"

class MediaMetadataFinderTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testMediaMetadataFinderCanFindMetadata();

private:
    QDir m_dataDir;
};

void MediaMetadataFinderTest::initTestCase()
{
    qRegisterMetaType<MediaMetadata>();

    m_dataDir = QDir(QFINDTESTDATA("testdata/default"));
    QVERIFY(!m_dataDir.isEmpty());
}

void MediaMetadataFinderTest::testMediaMetadataFinderCanFindMetadata()
{
    MediaMetadataFinder *finder = new MediaMetadataFinder(m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName1));
    QSignalSpy spy(finder, &MediaMetadataFinder::metadataFound);
    QThreadPool::globalInstance()->start(finder);

    spy.wait(10 * 1000);
    QCOMPARE(spy.count(), 1);

    const auto args = spy.takeFirst();
    QCOMPARE(args.at(0).toString(), m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName1));

    const auto metadata = args.at(1).value<MediaMetadata>();
#if HAVE_KF5KExiv2
    QTRY_COMPARE(metadata.title, QStringLiteral("DocumentName"));
    QTRY_COMPARE(metadata.author, QStringLiteral("KDE Community"));
#endif
    QTRY_COMPARE(metadata.resolution, QSize(15, 16));
}

QTEST_MAIN(MediaMetadataFinderTest)

#include "test_mediametadatafinder.moc"
