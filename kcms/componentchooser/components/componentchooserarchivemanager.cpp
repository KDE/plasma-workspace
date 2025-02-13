/*
    SPDX-FileCopyrightText: 2022 Thiago Sueto <herzenschein@gmail.com>
    SPDX-FileCopyrightText: 2022 Méven Car <meven@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "componentchooserarchivemanager.h"

ComponentChooserArchiveManager::ComponentChooserArchiveManager(QObject *parent)
    : ComponentChooser(parent,
                       QStringLiteral("application/zip"),
                       QStringLiteral("Archiving"),
                       QStringLiteral("org.kde.ark.desktop"),
                       i18n("Select default archive manager"))
{
}

static const QStringList archiveMimetypes{QStringLiteral("application/x-tar"),
                                          QStringLiteral("application/x-compressed-tar"),
                                          QStringLiteral("application/x-bzip-compressed-tar"),
                                          QStringLiteral("application/x-tarz"),
                                          QStringLiteral("application/x-xz-compressed-tar"),
                                          QStringLiteral("application/x-lzma-compressed-tar"),
                                          QStringLiteral("application/x-lzip-compressed-tar"),
                                          QStringLiteral("application/x-tzo"),
                                          QStringLiteral("application/x-lrzip-compressed-tar"),
                                          QStringLiteral("application/x-lz4-compressed-tar"),
                                          QStringLiteral("application/x-zstd-compressed-tar"),
                                          QStringLiteral("application/x-cd-image"),
                                          QStringLiteral("application/x-bcpio"),
                                          QStringLiteral("application/x-cpio"),
                                          QStringLiteral("application/x-cpio-compressed"),
                                          QStringLiteral("application/x-sv4cpio"),
                                          QStringLiteral("application/x-sv4crc"),
                                          QStringLiteral("application/x-source-rpm"),
                                          QStringLiteral("application/vnd.ms-cab-compressed"),
                                          QStringLiteral("application/x-xar"),
                                          QStringLiteral("application/x-iso9660-appimage"),
                                          QStringLiteral("application/x-archive"),
                                          QStringLiteral("application/vnd.rar"),
                                          QStringLiteral("application/x-rar"),
                                          QStringLiteral("application/x-7z-compressed"),
                                          QStringLiteral("application/zip"),
                                          QStringLiteral("application/x-compress"),
                                          QStringLiteral("application/gzip"),
                                          QStringLiteral("application/x-bzip"),
                                          QStringLiteral("application/x-lzma"),
                                          QStringLiteral("application/x-xz"),
                                          QStringLiteral("application/zstd"),
                                          QStringLiteral("application/x-lha")};

QStringList ComponentChooserArchiveManager::mimeTypes() const
{
    return archiveMimetypes;
}
