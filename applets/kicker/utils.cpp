/*
    SPDX-FileCopyrightText: 2025 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "utils.h"
#include <QDir>
#include <QFileInfo>

QUrl Utils::resolvedFile(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return url;
    }

    QFileInfo info(url.path());
    if (info.isSymLink()) {
        QString target = info.symLinkTarget();

        // If the target is relative, make it absolute relative to the link's directory
        if (QFileInfo(target).isRelative()) {
            target = QDir(info.absolutePath()).absoluteFilePath(target);
        }

        return QUrl::fromLocalFile(target);
    }

    return url;
}

#include <moc_utils.cpp>
