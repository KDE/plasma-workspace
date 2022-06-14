/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef VIDEOFINDER_H
#define VIDEOFINDER_H

#include <QObject>
#include <QRunnable>

/**
 * A runnable that finds all available videos in the specified paths.
 */
class VideoFinder : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit VideoFinder(const QStringList &paths, QObject *parent = nullptr);

    void run() override;

Q_SIGNALS:
    void videoFound(const QStringList &paths);

private:
    QStringList m_paths;
};

#endif // VIDEOFINDER_H
