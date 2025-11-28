/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QDir>
#include <QSignalSpy>
#include <QTest>

#include "../finder/mediametadatafinder.h"
#include "commontestdata.h"
#include "config-KExiv2.h"

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
    const auto metadata = MediaMetadata::read(m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName1));
#if HAVE_KExiv2
    QTRY_COMPARE(metadata.title, QStringLiteral("DocumentName"));
    QTRY_COMPARE(metadata.author, QStringLiteral("KDE Community"));
#endif
    QTRY_COMPARE(metadata.resolution, QSize(15, 16));
}

QTEST_MAIN(MediaMetadataFinderTest)

#include "test_mediametadatafinder.moc"
