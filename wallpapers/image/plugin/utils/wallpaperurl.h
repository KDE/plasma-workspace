/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <QUrl>
#include <qqmlintegration.h>

/**
 * The WallpaperUrl type provides a way to construct wallpaper urls that can be stored in the config.
 */
class WallpaperUrl : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit WallpaperUrl(QObject *parent = nullptr);

    Q_INVOKABLE QUrl make(const QString &raw, const QString &fragment = QString()) const;
    Q_INVOKABLE QUrl make(const QUrl &url, const QString &fragment = QString()) const;
};
