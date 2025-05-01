/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QFileInfo>

static QFileInfo findSymlinkTarget(const QFileInfo &info)
{
    if (!info.isSymLink()) {
        return info;
    }

    int count = 0;
    QFileInfo target(info.symLinkTarget());

    while (count < 10 && target.isSymLink()) {
        target = QFileInfo(target.symLinkTarget());
        count += 1;
    }

    if (QFileInfo(target).isSymLink()) {
        return info;
    }

    return target;
}
