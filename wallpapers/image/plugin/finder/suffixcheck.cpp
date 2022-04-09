/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "suffixcheck.h"

#include <mutex>

#include <QImageReader>
#include <QMimeDatabase>
#include <QSet>

static QStringList s_suffixes;
static std::mutex s_suffixMutex;

QStringList suffixes()
{
    std::lock_guard lock(s_suffixMutex);

    if (s_suffixes.empty()) {
        QSet<QString> suffixeSet;

        QMimeDatabase db;
        const auto supportedMimeTypes = QImageReader::supportedMimeTypes();

        for (const QByteArray &mimeType : supportedMimeTypes) {
            QMimeType mime(db.mimeTypeForName(QString::fromLatin1(mimeType)));
            const QStringList globPatterns = mime.globPatterns();

            for (const QString &pattern : globPatterns) {
                suffixeSet.insert(pattern);
            }
        }

        s_suffixes = suffixeSet.values();
    }

    return s_suffixes;
}

bool isAcceptableSuffix(const QString &suffix)
{
    // Despite its name, suffixes() returns a list of glob patterns.
    // Therefore the file suffix check needs to include the "*." prefix.
    const QStringList &globPatterns = suffixes();

    return globPatterns.contains(QLatin1String("*.") + suffix.toLower());
}
