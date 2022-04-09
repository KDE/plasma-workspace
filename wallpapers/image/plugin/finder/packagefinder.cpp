/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "packagefinder.h"

#include <QDir>

#include <KLocalizedString>
#include <KPackage/PackageLoader>

#include "distance.h"
#include "findsymlinktarget.h"
#include "suffixcheck.h"

PackageFinder::PackageFinder(const QStringList &paths, const QSize &targetSize, QObject *parent)
    : QObject(parent)
    , m_paths(paths)
    , m_targetSize(targetSize)
{
}

void PackageFinder::run()
{
    QList<KPackage::Package> packages;
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

        if (package.isValid() && package.metadata().isValid()) {
            // Check if there are any available images.
            QDir imageDir(package.filePath("images"));
            imageDir.setFilter(QDir::Files | QDir::Readable);
            imageDir.setNameFilters(suffixes());

            if (imageDir.entryInfoList().empty()) {
                // This is an empty package. Skip it.
                folders << folderPath;
                return true;
            }

            findPreferredImageInPackage(package, m_targetSize);
            packages << package;
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

void PackageFinder::findPreferredImageInPackage(KPackage::Package &package, const QSize &targetSize)
{
    if (!package.isValid()) {
        return;
    }

    QSize tSize = targetSize;

    if (tSize.isEmpty()) {
        tSize = QSize(1920, 1080);
    }

    // find preferred size
    QString preferred;
    {
        const QStringList images = package.entryList("images");

        if (images.empty()) {
            return;
        }

        float best = std::numeric_limits<float>::max();

        for (const QString &entry : images) {
            QSize candidate = resSize(QFileInfo(entry).baseName());

            if (candidate.isEmpty()) {
                continue;
            }

            const float dist = distance(candidate, tSize);

            if (preferred.isEmpty() || dist < best) {
                preferred = entry;
                best = dist;
            }
        }
    }

    package.removeDefinition("preferred");
    package.addFileDefinition("preferred", QStringLiteral("images/") + preferred, i18n("Recommended wallpaper file"));
}

QString PackageFinder::packageDisplayName(const KPackage::Package &b)
{
    const QString title = b.metadata().name();

    if (title.isEmpty()) {
        return QFileInfo(b.filePath("preferred")).completeBaseName();
    }

    return title;
}
