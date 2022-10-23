/*
    SPDX-FileCopyrightText: 2007 Aurélien Gâteau <agateau@kde.org>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mediametadatafinder.h"

#include <QFile>
#include <QImageReader>

#include "config-KF5KExiv2.h"
#if HAVE_KF5KExiv2
#include <KExiv2/KExiv2>
#endif

MediaMetadataFinder::MediaMetadataFinder(const QString &path, QObject *parent)
    : QObject(parent)
    , m_path(path)
{
}

void MediaMetadataFinder::run()
{
    MediaMetadata metadata;

    const QImageReader reader(m_path);
    metadata.resolution = reader.size();

#if HAVE_KF5KExiv2
    KExiv2Iface::KExiv2 exivImage(m_path);

    // Extract title from XPTitle
    {
        const QByteArray titleByte = exivImage.getExifTagData("Exif.Image.XPTitle");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        metadata.title = QString::fromUtf8(titleByte);
#else
        metadata.title = QString::fromUtf8(titleByte).chopped(std::min<qsizetype>(titleByte.size(), 1));
#endif
    }

    // Use documentName as title
    if (metadata.title.isEmpty()) {
        const QByteArray titleByte = exivImage.getExifTagData("Exif.Image.DocumentName");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        metadata.title = QString::fromUtf8(titleByte);
#else
        metadata.title = QString::fromUtf8(titleByte).chopped(std::min<qsizetype>(titleByte.size(), 1));
#endif
    }

    // Use description as title
    if (metadata.title.isEmpty()) {
        const QByteArray titleByte = exivImage.getExifTagData("Exif.Image.ImageDescription");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        metadata.title = QString::fromUtf8(titleByte);
#else
        metadata.title = QString::fromUtf8(titleByte).chopped(std::min<qsizetype>(titleByte.size(), 1));
#endif
    }

    // Extract author from artist
    {
        const QByteArray authorByte = exivImage.getExifTagData("Exif.Image.Artist");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        metadata.author = QString::fromUtf8(authorByte);
#else
        metadata.author = QString::fromUtf8(authorByte).chopped(std::min<qsizetype>(authorByte.size(), 1));
#endif
    }

    // Extract author from XPAuthor
    if (metadata.author.isEmpty()) {
        const QByteArray authorByte = exivImage.getExifTagData("Exif.Image.XPAuthor");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        metadata.author = QString::fromUtf8(authorByte);
#else
        metadata.author = QString::fromUtf8(authorByte).chopped(std::min<qsizetype>(authorByte.size(), 1));
#endif
    }

    // Extract author from copyright
    if (metadata.author.isEmpty()) {
        const QByteArray authorByte = exivImage.getExifTagData("Exif.Image.Copyright");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        metadata.author = QString::fromUtf8(authorByte);
#else
        metadata.author = QString::fromUtf8(authorByte).chopped(std::min<qsizetype>(authorByte.size(), 1));
#endif
    }
#endif

    Q_EMIT metadataFound(m_path, metadata);
}
