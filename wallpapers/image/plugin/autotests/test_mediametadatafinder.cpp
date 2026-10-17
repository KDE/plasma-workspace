/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QStringEncoder>
#include <QTemporaryDir>
#include <QTest>

#include "../finder/mediametadatafinder.h"
#include "commontestdata.h"
#include "config-KExiv2.h"
#if HAVE_KExiv2
#include <KExiv2/KExiv2>
#endif

class MediaMetadataFinderTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testMediaMetadataFinderCanFindMetadata();
    void testMediaMetadataFinderReadsTagEncodings_data();
    void testMediaMetadataFinderReadsTagEncodings();

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
}

void MediaMetadataFinderTest::testMediaMetadataFinderReadsTagEncodings_data()
{
    QTest::addColumn<QByteArray>("tag");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<bool>("isTitle");
    QTest::addColumn<QString>("expected");

    const QString title = QStringLiteral("\u00c9t\u00e9 \u591c");
    const QString author = QStringLiteral("Zo\u00eb");
    const auto utf16 = [](const QString &text) {
        QStringEncoder encoder(QStringEncoder::Utf16LE);
        return QByteArray(encoder(text));
    };
    const QByteArray utf16Terminator(2, '\0');
    const QByteArray utf8Terminator(1, '\0');

    // Windows writes the XP tags as UTF-16LE with a two byte null character at the end, see BUG 527048
    QTest::newRow("XPTitle UTF-16LE") << QByteArray("Exif.Image.XPTitle") << utf16(title) + utf16Terminator << true << title;
    QTest::newRow("XPAuthor UTF-16LE") << QByteArray("Exif.Image.XPAuthor") << utf16(author) + utf16Terminator << false << author;
    QTest::newRow("XPTitle UTF-16LE without terminator") << QByteArray("Exif.Image.XPTitle") << utf16(title) << true << title;
    QTest::newRow("XPTitle ASCII UTF-16LE without terminator")
        << QByteArray("Exif.Image.XPTitle") << utf16(QStringLiteral("Title")) << true << QStringLiteral("Title");
    // Some programs write UTF-8 into the XP tags
    QTest::newRow("XPTitle UTF-8") << QByteArray("Exif.Image.XPTitle") << title.toUtf8() + utf8Terminator << true << title;
    QTest::newRow("XPTitle UTF-8 without terminator") << QByteArray("Exif.Image.XPTitle") << title.toUtf8() << true << title;
    // The Exif ASCII tags often hold UTF-8
    QTest::newRow("Artist UTF-8") << QByteArray("Exif.Image.Artist") << author.toUtf8() + utf8Terminator << false << author;
}

void MediaMetadataFinderTest::testMediaMetadataFinderReadsTagEncodings()
{
#if HAVE_KExiv2
    QFETCH(QByteArray, tag);
    QFETCH(QByteArray, data);
    QFETCH(bool, isTitle);
    QFETCH(QString, expected);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("tags.jpg"));
    QVERIFY(QFile::copy(m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName1), path));
    QVERIFY(QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner));

    {
        KExiv2Iface::KExiv2 image(path);
        // The finder prefers these tags to the XP ones
        QVERIFY(image.removeExifTag("Exif.Image.DocumentName", false));
        QVERIFY(image.removeExifTag("Exif.Image.Artist", false));
        QVERIFY(image.setExifTagData(tag.constData(), data, false));
        QVERIFY(image.applyChanges());
    }

    const auto metadata = MediaMetadata::read(path);
    QCOMPARE(isTitle ? metadata.title : metadata.author, expected);
#else
    QSKIP("Built without KExiv2");
#endif
}

QTEST_MAIN(MediaMetadataFinderTest)

#include "test_mediametadatafinder.moc"
