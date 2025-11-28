/*
    SPDX-FileCopyrightText: 2007 Aurélien Gâteau <agateau@kde.org>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mediametadatafinder.h"

#include <QFile>
#include <QImageReader>

#include "config-KExiv2.h"
#if HAVE_KExiv2
#include <KExiv2/KExiv2>
#endif

MediaMetadata MediaMetadata::read(const QString &path)
{
    MediaMetadata metadata;

    const QImageReader reader(path);
    metadata.resolution = reader.size();

#if HAVE_KExiv2
    KExiv2Iface::KExiv2 exivImage(path);

    // Extract title from XPTitle
    {
        const QByteArray titleByte = exivImage.getExifTagData("Exif.Image.XPTitle");
        metadata.title = QString::fromUtf8(titleByte).chopped(std::min<qsizetype>(titleByte.size(), 1));
    }

    // Use documentName as title
    if (metadata.title.isEmpty()) {
        const QByteArray titleByte = exivImage.getExifTagData("Exif.Image.DocumentName");
        metadata.title = QString::fromUtf8(titleByte).chopped(std::min<qsizetype>(titleByte.size(), 1));
    }

    // Extract author from artist
    {
        const QByteArray authorByte = exivImage.getExifTagData("Exif.Image.Artist");
        metadata.author = QString::fromUtf8(authorByte).chopped(std::min<qsizetype>(authorByte.size(), 1));
    }

    // Extract author from XPAuthor
    if (metadata.author.isEmpty()) {
        const QByteArray authorByte = exivImage.getExifTagData("Exif.Image.XPAuthor");
        metadata.author = QString::fromUtf8(authorByte).chopped(std::min<qsizetype>(authorByte.size(), 1));
    }

    // Extract author from copyright
    if (metadata.author.isEmpty()) {
        const QByteArray authorByte = exivImage.getExifTagData("Exif.Image.Copyright");
        metadata.author = QString::fromUtf8(authorByte).chopped(std::min<qsizetype>(authorByte.size(), 1));
    }
#endif

    return metadata;
}
