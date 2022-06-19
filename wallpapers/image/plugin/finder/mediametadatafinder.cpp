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
    metadata.title = QString::fromUtf8(exivImage.getExifTagData("Exif.Image.XPTitle"));

    // Use documentName as title
    if (metadata.title.isEmpty()) {
        metadata.title = QString::fromUtf8(exivImage.getExifTagData("Exif.Image.DocumentName"));
    }

    // Use description as title
    if (metadata.title.isEmpty()) {
        metadata.title = QString::fromUtf8(exivImage.getExifTagData("Exif.Image.ImageDescription"));
    }

    // Extract author from artist
    metadata.author = QString::fromUtf8(exivImage.getExifTagData("Exif.Image.Artist"));

    // Extract author from XPAuthor
    if (metadata.author.isEmpty()) {
        metadata.author = QString::fromUtf8(exivImage.getExifTagData("Exif.Image.XPAuthor"));
    }

    // Extract author from copyright
    if (metadata.author.isEmpty()) {
        metadata.author = QString::fromUtf8(exivImage.getExifTagData("Exif.Image.Copyright"));
    }
#endif

    Q_EMIT metadataFound(m_path, metadata);
}
