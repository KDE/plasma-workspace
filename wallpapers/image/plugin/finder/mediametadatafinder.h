/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QSize>
#include <QString>

struct MediaMetadata {
    QString title;
    QString author;

    static MediaMetadata read(const QString &path);
};
