/*
    SPDX-FileCopyrightText: 2007 Aurélien Gâteau <agateau@kde.org>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mediametadatafinder.h"

#include "config-KExiv2.h"
#if HAVE_KExiv2
#include <KExiv2/KExiv2>
#endif

MediaMetadata MediaMetadata::read(const QString &path)
{
    MediaMetadata metadata;

#if HAVE_KExiv2
    KExiv2Iface::KExiv2 exivImage(path);

    // Extract title from XPTitle
    metadata.title = exivImage.getExifTagString("Exif.Image.XPTitle");

    // Use documentName as title
    if (metadata.title.isEmpty()) {
        metadata.title = exivImage.getExifTagString("Exif.Image.DocumentName");
    }

    // Extract author from artist
    metadata.author = exivImage.getExifTagString("Exif.Image.Artist");

    // Extract author from XPAuthor
    if (metadata.author.isEmpty()) {
        metadata.author = exivImage.getExifTagString("Exif.Image.XPAuthor");
    }

    // Extract author from copyright
    if (metadata.author.isEmpty()) {
        metadata.author = exivImage.getExifTagString("Exif.Image.Copyright");
    }
#endif

    return metadata;
}
