/*
    SPDX-FileCopyrightText: 2007 Aurélien Gâteau <agateau@kde.org>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mediametadatafinder.h"

#include "config-KExiv2.h"
#if HAVE_KExiv2
#include <KExiv2/KExiv2>

#include <QStringDecoder>

namespace
{
QString withoutTrailingNulls(QString text)
{
    while (text.endsWith(QChar(u'\0'))) {
        text.chop(1);
    }
    return text;
}

// An Exif ASCII tag, which many programs fill with UTF-8
QString asciiTag(const KExiv2Iface::KExiv2 &image, const char *name)
{
    return withoutTrailingNulls(QString::fromUtf8(image.getExifTagData(name)));
}

// The XP tags of Windows are UTF-16LE and end with a two byte null character.
// Some programs write UTF-8 into them instead.
QString xpTag(const KExiv2Iface::KExiv2 &image, const char *name)
{
    const QByteArray data = image.getExifTagData(name);
    const bool hasUtf16Terminator = data.size() % 2 == 0 && data.endsWith(QByteArrayView("\0\0", 2));
    if (!hasUtf16Terminator) {
        QStringDecoder utf8(QStringDecoder::Utf8);
        const QString text = withoutTrailingNulls(utf8(data));
        // UTF-16LE text in the Latin alphabet is also valid UTF-8, but it has a null character after each letter
        if (!utf8.hasError() && !text.contains(QChar(u'\0'))) {
            return text;
        }
    }
    QStringDecoder utf16(QStringDecoder::Utf16LE);
    return withoutTrailingNulls(utf16(data));
}
}
#endif

MediaMetadata MediaMetadata::read(const QString &path)
{
    MediaMetadata metadata;

#if HAVE_KExiv2
    KExiv2Iface::KExiv2 exivImage(path);

    // Extract title from XPTitle
    metadata.title = xpTag(exivImage, "Exif.Image.XPTitle");

    // Use documentName as title
    if (metadata.title.isEmpty()) {
        metadata.title = asciiTag(exivImage, "Exif.Image.DocumentName");
    }

    // Extract author from artist
    metadata.author = asciiTag(exivImage, "Exif.Image.Artist");

    // Extract author from XPAuthor
    if (metadata.author.isEmpty()) {
        metadata.author = xpTag(exivImage, "Exif.Image.XPAuthor");
    }

    // Extract author from copyright
    if (metadata.author.isEmpty()) {
        metadata.author = asciiTag(exivImage, "Exif.Image.Copyright");
    }
#endif

    return metadata;
}
