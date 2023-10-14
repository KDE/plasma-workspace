/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "suffixcheck.h"

#include <mutex>

#include <QImageReader>
#include <QMimeDatabase>
#include <QSet>

namespace
{
QStringList s_suffixes;
std::once_flag s_onceFlag;

void fillSuffixes()
{
    Q_ASSERT(s_suffixes.empty());

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
}

const QStringList &suffixes()
{
    std::call_once(s_onceFlag, &fillSuffixes);
    return s_suffixes;
}

bool isAcceptableSuffix(QString &&suffix)
{
    // Despite its name, suffixes() returns a list of glob patterns.
    // Therefore the file suffix check needs to include the "*." prefix.
    std::call_once(s_onceFlag, &fillSuffixes);
    return s_suffixes.contains(QLatin1String("*.%1").arg(std::move(suffix).toLower()));
}
