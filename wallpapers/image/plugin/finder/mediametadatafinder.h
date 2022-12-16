/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <QRunnable>
#include <QSize>

struct MediaMetadata {
    QString title;
    QString author;
    QSize resolution;
};
Q_DECLARE_METATYPE(MediaMetadata)

/**
 * A runnable that helps find the metadata of an image or a video.
 */
class MediaMetadataFinder : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit MediaMetadataFinder(const QString &path, QObject *parent = nullptr);

    void run() override;

Q_SIGNALS:
    void metadataFound(const QString &path, const MediaMetadata &metadata);

private:
    QString m_path;
};
