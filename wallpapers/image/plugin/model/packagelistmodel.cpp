/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "packagelistmodel.h"

#include <QDir>
#include <QGuiApplication>
#include <QPalette>
#include <QPixmap>
#include <QStandardPaths>
#include <QThreadPool>
#include <QUrlQuery>

#include <KAboutData>
#include <KPackage/PackageLoader>

#include "../finder/packagefinder.h"
#include "../finder/suffixcheck.h"

PackageListModel::PackageListModel(const QBindable<QSize> &bindableTargetSize, const QBindable<bool> &bindableUsedInConfig, QObject *parent)
    : AbstractImageListModel(bindableTargetSize, bindableUsedInConfig, parent)
{
    qRegisterMetaType<QList<KPackage::Package>>();
}

int PackageListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_packages.size();
}

QVariant PackageListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    const WallpaperPackage &b = m_packages.at(index.row());

    if (!b.package().isValid()) {
        Q_UNREACHABLE(); // Should be already filtered out by the finder
    }

    switch (role) {
    case Qt::DisplayRole:
        return b.displayName();

    case PreviewRole: {
        QUrl previewUrl(QStringLiteral("image://wallpaper-preview"));
        previewUrl.setQuery({
            std::make_pair(QStringLiteral("package"), b.package().path()),
        });
        return previewUrl;
    }

    case AuthorRole: {
        if (!b.package().metadata().authors().empty()) {
            return b.package().metadata().authors().at(0).name();
        }

        return QString();
    }

    case PathRole: {
        if (qGray(qGuiApp->palette().window().color().rgb()) < 192) {
            const QString darkPath = b.package().filePath(QByteArrayLiteral("preferredDark"));
            if (!darkPath.isEmpty()) {
                return QUrl::fromLocalFile(darkPath);
            }
        }

        return QUrl::fromLocalFile(b.package().filePath("preferred"));
    }

    case PackageNameRole:
        return b.package().path();

    case RemovableRole: {
        const QString path = b.package().path();

        return path.startsWith(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/wallpapers/"))
            || m_removableWallpapers.contains(path);
    }

    case PendingDeletionRole:
        return m_pendingDeletion.value(b.package().path(), false);

    case SelectorsRole:
        return b.selectors();
    }
    Q_UNREACHABLE();
}

bool PackageListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    if (role == PendingDeletionRole) {
        const WallpaperPackage &b = m_packages.at(index.row());
        m_pendingDeletion[b.package().path()] = value.toBool();

        Q_EMIT dataChanged(index, index, {PendingDeletionRole});
        return true;
    }

    return false;
}

static QString normalizeDirName(const QString &filePath)
{
    return filePath.endsWith(QDir::separator()) ? filePath : filePath + QDir::separator();
}

int PackageListModel::indexOf(const QUrl &url) const
{
    const QString path = normalizeDirName(url.toLocalFile());
    const auto it = std::find_if(m_packages.cbegin(), m_packages.cend(), [&path](const WallpaperPackage &p) {
        return path == p.package().path();
    });

    if (it == m_packages.cend()) {
        return -1;
    }

    return std::distance(m_packages.cbegin(), it);
}

void PackageListModel::load(const QStringList &customPaths)
{
    if (m_loading || customPaths.empty()) {
        return;
    }

    AbstractImageListModel::load(customPaths);

    auto *finder = new PackageFinder(m_customPaths, m_targetSize);
    connect(finder, &PackageFinder::packageFound, this, &PackageListModel::slotHandlePackageFound);
    QThreadPool::globalInstance()->start(finder);
}

QStringList PackageListModel::addBackground(const QUrl &url)
{
    if (url.isEmpty() || indexOf(url) >= 0) {
        return {};
    }

    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));
    package.setPath(url.toLocalFile());

    if (!package.isValid() || !package.metadata().isValid()) {
        return {};
    }

    // Check if there are any available images.
    QDir imageDir(package.filePath("images"));
    imageDir.setFilter(QDir::Files | QDir::Readable);
    imageDir.setNameFilters(suffixes());

    if (imageDir.entryInfoList().empty()) {
        // This is an empty package. Skip it.
        return {};
    }

    PackageFinder::findPreferredImageInPackage(package, m_targetSize);

    if (m_usedInConfig) {
        beginInsertRows(QModelIndex(), 0, 0);

        m_removableWallpapers.prepend(package.path());
        m_packages.prepend(package);

        endInsertRows();
    } else {
        // In a slideshow, append to last so the random order can be kept
        const int count = rowCount();
        beginInsertRows(QModelIndex(), count, count);

        m_removableWallpapers.append(package.path());
        m_packages.append(package);

        endInsertRows();
    }

    return {package.path()};
}

QStringList PackageListModel::removeBackground(const QUrl &url)
{
    QStringList results;

    if (url.isEmpty()) {
        return results;
    }

    const int idx = indexOf(url);
    if (idx < 0) {
        return results;
    }

    beginRemoveRows(QModelIndex(), idx, idx);

    m_pendingDeletion.remove(m_packages.at(idx).package().path());
    m_removableWallpapers.removeOne(m_packages.at(idx).package().path());
    results.append(m_packages.takeAt(idx).package().path());

    // Uninstall local package
    const QString path = url.toLocalFile();
    if (path.startsWith(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/wallpapers/"))) {
        QDir f(path);

        if (f.exists()) {
            f.removeRecursively();
        }
    }

    endRemoveRows();

    return results;
}

void PackageListModel::slotHandlePackageFound(const QList<WallpaperPackage> &packages)
{
    beginResetModel();

    m_packages = packages;
    clearCache();

    endResetModel();

    m_loading = false;
    Q_EMIT loaded(this);
}
