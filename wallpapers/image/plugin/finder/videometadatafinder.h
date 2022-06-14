/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef VIDEOMETADATAFINDER_H
#define VIDEOMETADATAFINDER_H

#include <QObject>
#include <QRunnable>
#include <QSize>

struct VideoMetadata {
    QString title;
    QString author;
    QSize resolution;
};
Q_DECLARE_METATYPE(VideoMetadata)

/**
 * A runnable that helps find the metadata of a video.
 */
class VideoMetadataFinder : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit VideoMetadataFinder(const QString &path, QObject *parent = nullptr);

    void run() override;

Q_SIGNALS:
    void metadataFound(const QString &path, const VideoMetadata &metadata);

private:
    QString m_path;
};

#endif // VIDEOMETADATAFINDER_H
