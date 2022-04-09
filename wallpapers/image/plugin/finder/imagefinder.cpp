/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "imagefinder.h"

#include <QDir>
#include <QImageReader>

#include "findsymlinktarget.h"
#include "suffixcheck.h"

ImageFinder::ImageFinder(const QStringList &paths, QObject *parent)
    : QObject(parent)
    , m_paths(paths)
{
}

void ImageFinder::run()
{
    QStringList images;

    QDir dir;
    dir.setFilter(QDir::AllDirs | QDir::Files | QDir::Readable);
    dir.setNameFilters(suffixes());

    const auto filterCondition = [](const QFileInfo &info) {
        return info.baseName() != QLatin1String("screenshot") && !info.absoluteFilePath().contains(QLatin1String("contents/images/"));
    };
    int i;

    for (i = 0; i < m_paths.size(); ++i) {
        const QString &path = m_paths.at(i);
        const QString target = findSymlinkTarget(path);
        const QFileInfo info(target);

        if (!info.exists() || !filterCondition(info)) {
            // is in a package
            continue;
        }

        if (info.isFile()) {
            if (isAcceptableSuffix(info.suffix()) && !info.isSymLink()) {
                images.append(target);
            }

            continue;
        }

        dir.setPath(path);
        const QFileInfoList files = dir.entryInfoList();

        for (const QFileInfo &wp : files) {
            const QString t = findSymlinkTarget(wp);

            if (wp.isFile()) {
                if (filterCondition(wp) && !wp.isSymLink()) {
                    images.append(t);
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

    images.removeAll(QString());
    images.removeDuplicates();

    Q_EMIT imageFound(images);
}
