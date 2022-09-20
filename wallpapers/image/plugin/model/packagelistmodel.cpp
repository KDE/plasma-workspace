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

#include <KAboutData>
#include <KPackage/PackageLoader>

#include "../finder/packagefinder.h"
#include "../finder/suffixcheck.h"

PackageListModel::PackageListModel(const QSize &targetSize, QObject *parent)
    : AbstractImageListModel(targetSize, parent)
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

    const KPackage::Package &b = m_packages.at(index.row());

    if (!b.isValid()) {
        Q_UNREACHABLE(); // Should be already filtered out by the finder
    }

    switch (role) {
    case Qt::DisplayRole:
        return PackageFinder::packageDisplayName(b);

    case ScreenshotRole: {
        QStringList paths{b.filePath(QByteArrayLiteral("preferred"))};
        const QString darkPath = b.filePath(QByteArrayLiteral("preferredDark"));

        if (!darkPath.isEmpty()) {
            paths.append(darkPath);
        }

        QPixmap *cachedPreview = m_imageCache.object(paths);

        if (cachedPreview) {
            return *cachedPreview;
        }

        asyncGetPreview(paths, QPersistentModelIndex(index));

        return QVariant();
    }

    case AuthorRole: {
        if (!b.metadata().authors().empty()) {
            return b.metadata().authors().at(0).name();
        }

        return QString();
    }

    case ResolutionRole: {
        const QString path = b.filePath("preferred");

        QSize *size = m_imageSizeCache.object(path);

        if (size && size->isValid()) {
            return QStringLiteral("%1x%2").arg(size->width()).arg(size->height());
        }

        asyncGetMediaMetadata(path, QPersistentModelIndex(index));

        return QString();
    }

    case PathRole: {
        if (qGray(qGuiApp->palette().window().color().rgb()) < 192) {
            const QString darkPath = b.filePath(QByteArrayLiteral("preferredDark"));
            if (!darkPath.isEmpty()) {
                return QUrl::fromLocalFile(darkPath);
            }
        }

        return QUrl::fromLocalFile(b.filePath("preferred"));
    }

    case PackageNameRole:
        return b.path();

    case RemovableRole: {
        const QString path = b.path();

        return path.startsWith(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/wallpapers/"))
            || m_removableWallpapers.contains(path);
    }

    case PendingDeletionRole:
        return m_pendingDeletion.value(b.path(), false);
    }
    Q_UNREACHABLE();
}

bool PackageListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    if (role == PendingDeletionRole) {
        const KPackage::Package &b = m_packages.at(index.row());
        m_pendingDeletion[b.path()] = value.toBool();

        Q_EMIT dataChanged(index, index, {PendingDeletionRole});
        return true;
    }

    return false;
}

int PackageListModel::indexOf(const QString &_path) const
{
    QString path = _path.endsWith(QDir::separator()) ? _path : _path + QDir::separator();

    if (path.startsWith(QLatin1String("file://"))) {
        path.remove(0, 7);
    }

    const auto it = std::find_if(m_packages.cbegin(), m_packages.cend(), [&path](const KPackage::Package &p) {
        return path == p.path();
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

    m_customPaths = customPaths;
    m_customPaths.removeDuplicates();

    PackageFinder *finder = new PackageFinder(m_customPaths, m_targetSize);
    connect(finder, &PackageFinder::packageFound, this, &PackageListModel::slotHandlePackageFound);
    QThreadPool::globalInstance()->start(finder);

    m_loading = true;
}

QStringList PackageListModel::addBackground(const QString &path)
{
    if (path.isEmpty() || indexOf(path) >= 0 || !QFileInfo(path).isDir()) {
        return {};
    }

    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));
    package.setPath(path);

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

    beginInsertRows(QModelIndex(), 0, 0);

    m_removableWallpapers.prepend(package.path());
    m_packages.prepend(package);

    endInsertRows();

    return {package.path()};
}

QStringList PackageListModel::removeBackground(const QString &_path)
{
    QStringList results;

    if (_path.isEmpty()) {
        return results;
    }

    const QString path = _path.endsWith(QDir::separator()) ? _path : _path + QDir::separator();

    const int idx = indexOf(path);

    if (idx < 0) {
        return results;
    }

    beginRemoveRows(QModelIndex(), idx, idx);

    m_pendingDeletion.remove(m_packages.at(idx).path());
    m_removableWallpapers.removeOne(m_packages.at(idx).path());
    results.append(m_packages.takeAt(idx).path());

    // Uninstall local package
    if (path.startsWith(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/wallpapers/"))) {
        QDir f(path);

        if (f.exists()) {
            f.removeRecursively();
        }
    }

    endRemoveRows();

    return results;
}

void PackageListModel::slotHandlePackageFound(const QList<KPackage::Package> &packages)
{
    beginResetModel();

    m_packages = packages;
    clearCache();

    endResetModel();

    m_loading = false;
    Q_EMIT loaded(this);
}
