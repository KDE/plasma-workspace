/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "wallpaperurl.h"

WallpaperUrl::WallpaperUrl(QObject *parent)
    : QObject(parent)
{
}

QUrl WallpaperUrl::make(const QString &raw, const QString &fragment) const
{
    return make(QUrl::fromUserInput(raw), fragment);
}

QUrl WallpaperUrl::make(const QUrl &url, const QString &fragment) const
{
    QUrl ret = url;
    if (!fragment.isEmpty()) {
        ret.setFragment(fragment);
    }
    return ret;
}

#include "moc_wallpaperurl.cpp"
