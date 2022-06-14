/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "videofinder.h"

#include <QDir>

#include "findsymlinktarget.h"
#include "suffixcheck.h"

VideoFinder::VideoFinder(const QStringList &paths, QObject *parent)
    : QObject(parent)
    , m_paths(paths)
{
}

void VideoFinder::run()
{
    QStringList videos;

    QDir dir;
    dir.setFilter(QDir::AllDirs | QDir::Files | QDir::Readable);
    dir.setNameFilters(videoSuffixes());

    const auto filterCondition = [](const QFileInfo &info) {
        const QString path = info.absoluteFilePath();

        return info.baseName() != QLatin1String("screenshot") && !path.contains(QLatin1String("contents/images/"))
            && !path.contains(QLatin1String("contents/images_dark/"));
    };
    int i;

    for (i = 0; i < m_paths.size(); ++i) {
        const QString &path = m_paths.at(i);
        const QString target = findSymlinkTarget(QFileInfo(path));
        const QFileInfo info(target);

        if (!info.exists() || !filterCondition(info)) {
            // is in a package
            continue;
        }

        if (info.isFile()) {
            if (isAcceptableVideoSuffix(info.suffix()) && !info.isSymLink()) {
                videos.append(target);
            }

            continue;
        }

        dir.setPath(path);
        const QFileInfoList files = dir.entryInfoList();

        for (const QFileInfo &wp : files) {
            const QString t = findSymlinkTarget(wp);

            if (wp.isFile()) {
                if (filterCondition(wp) && !wp.isSymLink()) {
                    videos.append(t);
                }
            } else {
                const QString name = wp.fileName();

                if (name.startsWith(QLatin1Char('.'))) {
                    continue;
                }

                // add this to the directories we should be looking at
                m_paths.append(wp.filePath());
            }
        }
    }

    videos.removeAll(QString());
    videos.removeDuplicates();

    Q_EMIT videoFound(videos);
}
