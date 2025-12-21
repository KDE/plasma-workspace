/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "filedialognamefilters.h"

#include <KLocalizedString>

#include <QImageReader>

FileDialogNameFilters::FileDialogNameFilters(QObject *parent)
    : QObject(parent)
{
}

QString FileDialogNameFilters::imageFiles() const
{
    const auto formats = QImageReader::supportedImageFormats();

    QStringList wildcards;
    wildcards.reserve(formats.size());
    for (const QByteArray &format : formats) {
        wildcards.append(QString::fromUtf8("*." + format));
    }

    return i18nc("Name filter for file dialogs: File type (file extension wildcards)", "Image Files (%1)", wildcards.join(QLatin1Char(' ')));
}

#include "moc_filedialognamefilters.cpp"
