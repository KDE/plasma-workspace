/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef FINDSYMLINKTARGET_H
#define FINDSYMLINKTARGET_H

#include <QFileInfo>

static QString findSymlinkTarget(const QFileInfo &info)
{
    if (!info.isSymLink()) {
        return info.filePath();
    }

    int count = 0;
    QString target = info.symLinkTarget();

    while (count < 10 && QFileInfo(target).isSymLink()) {
        target = info.symLinkTarget();
        count += 1;
    }

    if (QFileInfo(target).isSymLink()) {
        return QString();
    }

    return target;
}

#endif
