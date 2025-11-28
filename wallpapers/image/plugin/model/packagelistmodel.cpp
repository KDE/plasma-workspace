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
#include <QUrlQuery>
#include <QtConcurrent>

#include <KAboutData>
#include <KPackage/PackageLoader>
#include <algorithm>

#include "../finder/packagefinder.h"
#include "../finder/suffixcheck.h"

PackageListModel::PackageListModel(const QBindable<bool> &bindableUsedInConfig, QObject *parent)
    : AbstractImageListModel(bindableUsedInConfig, parent)
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
        return {};
    }

    const WallpaperPackage &b = m_packages.at(index.row());

    if (!b.package().isValid()) {
        Q_UNREACHABLE(); // Should be already filtered out by the finder
    }

    switch (role) {
    case Qt::DisplayRole:
        return b.package().metadata().name();

    case PreviewRole: {
        QString previewUri;
        previewUri.append(QLatin1String("image://wallpaper-preview/package/"));
        previewUri.append(b.package().path());
        return previewUri;
    }

    case AuthorRole: {
        if (!b.package().metadata().authors().empty()) {
            return b.package().metadata().authors().at(0).name();
        }

        return QString();
    }

    case SourceRole:
        return QUrl::fromLocalFile(b.package().path());

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

QUrl PackageListModel::effectiveSource(const QModelIndex &index, const QSize &targetSize) const
{
    if (!index.isValid()) {
        return {};
    }

    KPackage::Package package = m_packages.at(index.row()).package();
    WallpaperPackage::findPreferredImageInPackage(package, targetSize);

    if (qGray(qGuiApp->palette().window().color().rgb()) < 192) {
        const QString darkPath = package.filePath(QByteArrayLiteral("preferredDark"));
        if (!darkPath.isEmpty()) {
            return QUrl::fromLocalFile(darkPath);
        }
    }

    return QUrl::fromLocalFile(package.filePath("preferred"));
}

static QString normalizeDirName(const QString &filePath)
{
    return filePath.endsWith(QDir::separator()) ? filePath : filePath + QDir::separator();
}

int PackageListModel::indexOf(const QUrl &url) const
{
    const QString path = normalizeDirName(url.toLocalFile());
    const auto it = std::ranges::find_if(m_packages, [&path](const WallpaperPackage &p) {
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

    QtConcurrent::run(WallpaperPackage::findAll, m_customPaths).then(this, [this](const QList<WallpaperPackage> &packages) {
        beginResetModel();
        m_packages = packages;
        endResetModel();

        m_loading = false;
        Q_EMIT loaded(this);
    });
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
