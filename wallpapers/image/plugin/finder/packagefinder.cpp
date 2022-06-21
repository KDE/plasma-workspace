/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "packagefinder.h"

#include <QDir>

#include <KPackage/PackageLoader>

#include "findsymlinktarget.h"
#include "imagepackage.h"
#include "suffixcheck.h"

PackageFinder::PackageFinder(const QStringList &paths, const QSize &targetSize, QObject *parent)
    : QObject(parent)
    , m_paths(paths)
    , m_targetSize(targetSize)
{
}

void PackageFinder::run()
{
    QList<KPackage::ImagePackage> packages;
    QStringList folders;

    QDir dir;
    dir.setFilter(QDir::Dirs | QDir::Readable);

    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));

    const auto addPackage = [this, &package, &packages, &folders](const QString &_folderPath) {
        const QString folderPath = _folderPath.endsWith(QDir::separator()) ? _folderPath : _folderPath + QDir::separator();

        if (folders.contains(folderPath)) {
            // The folder has been added, return true to skip it.
            return true;
        }

        if (!QFile::exists(folderPath + QLatin1String("/metadata.desktop")) && !QFile::exists(folderPath + QLatin1String("/metadata.json"))) {
            return false;
        }

        package.setPath(folderPath);
        KPackage::ImagePackage imagePackage(package, m_targetSize);

        if (package.isValid()) {
            if (imagePackage.isValid()) {
                packages << imagePackage;
            }
            folders << folderPath;

            return true;
        }

        return false; // Not found
    };

    int i;

    for (i = 0; i < m_paths.size(); ++i) {
        const QString &path = m_paths.at(i);
        const QFileInfo info(path);

        if (!info.exists() || info.isFile()) {
            continue;
        }

        // Check the path itself is a package
        if (addPackage(path)) {
            continue;
        }

        dir.setPath(path);
        const QFileInfoList files = dir.entryInfoList();

        for (const QFileInfo &wp : files) {
            const QString folderPath = findSymlinkTarget(wp);

            if (wp.fileName().startsWith(QLatin1Char('.'))) {
                continue;
            }

            if (!addPackage(folderPath)) {
                // Add this to the directories we should be looking at
                m_paths.append(folderPath);
            }
        }
    }

    Q_EMIT packageFound(packages);
}

QString PackageFinder::packageDisplayName(const KPackage::Package &b)
{
    const QString title = b.metadata().name();

    if (title.isEmpty()) {
        return QFileInfo(b.filePath("preferred")).completeBaseName();
    }

    return title;
}
