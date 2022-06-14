/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "videometadatafinder.h"

#include <QEventLoop>
#include <QMediaPlayer>

VideoMetadataFinder::VideoMetadataFinder(const QString &path, QObject *parent)
    : QObject(parent)
    , m_path(path)
{
}

void VideoMetadataFinder::run()
{
    QEventLoop loop;
    QMediaPlayer player;
    VideoMetadata metadata;

    loop.connect(&player, &QMediaPlayer::mediaStatusChanged, this, [&loop, &player, &metadata] {
        switch (player.mediaStatus()) {
        case QMediaPlayer::LoadingMedia:
            return;
        case QMediaPlayer::LoadedMedia: {
            metadata.title = player.metaData(QStringLiteral("Title")).toString();
            metadata.author = player.metaData(QStringLiteral("Author")).toString();
            if (metadata.author.isEmpty()) {
                metadata.author = player.metaData(QStringLiteral("Copyright")).toString();
            }
            metadata.resolution = player.metaData(QStringLiteral("Resolution")).toSize();
            break;
        }
        default:
            break;
        }
        loop.quit();
    });

    player.setMedia(QUrl::fromLocalFile(m_path));
    loop.exec();

    Q_EMIT metadataFound(m_path, metadata);
}
