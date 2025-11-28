/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "packagefinder.h"

#include <limits>

#include <QDir>
#include <QImageReader>

#include <KPackage/PackageLoader>

#include "findsymlinktarget.h"
#include "suffixcheck.h"

namespace
{
/**
 * Computes difference of two areas
 */
double distance(const QSize &size, const QSize &desired)
{
    const double desiredAspectRatio = (desired.height() > 0) ? desired.width() / static_cast<double>(desired.height()) : 0;
    const double candidateAspectRatio = (size.height() > 0) ? size.width() / static_cast<double>(size.height()) : std::numeric_limits<double>::max();

    double delta = size.width() - desired.width();
    delta = delta >= 0.0 ? delta : -delta * 2; // Penalize for scaling up

    return std::abs(candidateAspectRatio - desiredAspectRatio) * 25000 + delta;
}

/**
 * @return size from the filename
 */
QSize resSize(QStringView str)
{
    const int index = str.indexOf(QLatin1Char('x'));

    if (index != -1) {
        return {str.left(index).toInt(), str.mid(index + 1).toInt()};
    }

    return {};
}
}

WallpaperPackage::WallpaperPackage(const KPackage::Package &package, const QStringList &selectors)
    : m_package(package)
    , m_selectors(selectors)
{
}

KPackage::Package WallpaperPackage::package() const
{
    return m_package;
}

QStringList WallpaperPackage::selectors() const
{
    return m_selectors;
}

static bool isDayNightSupported(const QString &lightFilePath, const QString &darkFilePath)
{
    if (QImageReader(lightFilePath).supportsAnimation()) {
        return false;
    } else if (QImageReader(darkFilePath).supportsAnimation()) {
        return false;
    }
    return true;
}

std::optional<WallpaperPackage> WallpaperPackage::from(const QString &filePath)
{
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));
    package.setPath(filePath);
    return from(package);
}

std::optional<WallpaperPackage> WallpaperPackage::from(const KPackage::Package &package)
{
    if (!package.isValid() || !package.metadata().isValid()) {
        return std::nullopt;
    }

    const QDir imageDirectory(package.filePath("images"));
    const QFileInfoList imageFiles = imageDirectory.entryInfoList(suffixes(), QDir::Files | QDir::Readable);
    if (imageFiles.isEmpty()) {
        return std::nullopt;
    }

    QStringList selectors;

    const QDir darkImagesDirectory(package.filePath("images_dark"));
    const QFileInfoList darkImageFiles = darkImagesDirectory.entryInfoList(suffixes(), QDir::Files | QDir::Readable);
    if (!darkImageFiles.isEmpty()) {
        selectors << QStringLiteral("dark-light");

        if (isDayNightSupported(imageFiles.first().absoluteFilePath(), darkImageFiles.first().absoluteFilePath())) {
            selectors << QStringLiteral("day-night");
        }
    }

    return WallpaperPackage(package, selectors);
}

QList<WallpaperPackage> WallpaperPackage::findAll(const QStringList &paths)
{
    QList<WallpaperPackage> packages;
    QStringList folders;

    QDir dir;
    dir.setFilter(QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot);

    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));

    const auto addPackage = [&package, &packages, &folders](const QString &_folderPath) {
        const QString folderPath = findSymlinkTarget(QFileInfo(_folderPath)).absoluteFilePath();

        if (folders.contains(folderPath)) {
            // The folder has been added, return true to skip it.
            return true;
        }

        if (!QFile::exists(folderPath + QLatin1String("/metadata.desktop")) && !QFile::exists(folderPath + QLatin1String("/metadata.json"))) {
            folders << folderPath;
            return false;
        }

        package.setPath(folderPath);

        if (const auto wallpaper = WallpaperPackage::from(package)) {
            packages << *wallpaper;
            folders << folderPath;
            return true;
        }

        folders << folderPath;
        return false; // Not found
    };

    int i;

    QStringList visitQueue = paths;
    for (i = 0; i < visitQueue.size(); ++i) {
        const QString &path = visitQueue.at(i);
        const QFileInfo info(path);

        if (!info.isDir()) {
            continue;
        }

        // Check the path itself is a package
        if (addPackage(path)) {
            continue;
        }

        dir.setPath(path);
        const QFileInfoList files = dir.entryInfoList();

        for (const QFileInfo &wp : files) {
            if (!addPackage(wp.filePath())) {
                // Add this to the directories we should be looking at
                visitQueue.append(wp.filePath());
            }
        }
    }

    return packages;
}

void WallpaperPackage::findPreferredImageInPackage(KPackage::Package &package, const QSize &targetSize)
{
    if (!package.isValid()) {
        return;
    }

    QSize tSize = targetSize;

    if (tSize.isEmpty()) {
        tSize = QSize(1920, 1080);
    }

    // find preferred size
    auto findBestMatch = [&package, &tSize](const QByteArray &folder) {
        QString preferred;
        const QStringList images = package.entryList(folder);

        if (images.empty()) {
            return preferred;
        }

        double best = std::numeric_limits<double>::max();

        for (const QString &entry : images) {
            QSize candidate = resSize(QFileInfo(entry).baseName());

            if (candidate.isEmpty()) {
                continue;
            }

            const double dist = distance(candidate, tSize);

            if (preferred.isEmpty() || dist < best) {
                preferred = entry;
                best = dist;
            }
        }

        return preferred;
    };

    const QString preferred = findBestMatch(QByteArrayLiteral("images"));
    const QString preferredDark = findBestMatch(QByteArrayLiteral("images_dark"));

    package.removeDefinition("preferred");
    package.addFileDefinition("preferred", QStringLiteral("images/%1").arg(preferred));

    if (!preferredDark.isEmpty()) {
        package.removeDefinition("preferredDark");
        package.addFileDefinition("preferredDark", QStringLiteral("images_dark/%1").arg(preferredDark));
    }
}
