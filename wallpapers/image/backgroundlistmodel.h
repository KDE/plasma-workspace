/***************************************************************************
 *   Copyright 2007 Paolo Capriotti <p.capriotti@gmail.com>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#ifndef BACKGROUNDLISTMODEL_H
#define BACKGROUNDLISTMODEL_H

#include "image.h"

#include <QAbstractListModel>
#include <QCache>
#include <QMutex>
#include <QPixmap>
#include <QRunnable>
#include <QSet>
#include <QThread>

#include <KDirWatch>
#include <KFileItem>

#include <KPackage/PackageStructure>

class Image;

class ImageSizeFinder : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit ImageSizeFinder(const QString &path, QObject *parent = nullptr);
    void run() override;

Q_SIGNALS:
    void sizeFound(const QString &path, const QSize &size);

private:
    QString m_path;
};

class BackgroundListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum {
        AuthorRole = Qt::UserRole,
        ScreenshotRole,
        ResolutionRole,
        PathRole,
        PackageNameRole,
        RemovableRole,
        PendingDeletionRole,
        ToggleRole,
    };

    static const int BLUR_INCREMENT = 9;
    static const int MARGIN = 6;

    BackgroundListModel(Image *listener, QObject *parent);
    ~BackgroundListModel() override;

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    KPackage::Package package(int index) const;

    void reload();
    void reload(const QStringList &selected);
    void addBackground(const QString &path);
    void removeBackground(const QString &path);
    Q_INVOKABLE int indexOf(const QString &path) const;
    virtual bool contains(const QString &bg) const;

    int count() const
    {
        return m_packages.size();
    }

    Q_INVOKABLE void openContainingFolder(int rowIndex);
    Q_INVOKABLE void setPendingDeletion(int rowIndex, bool pendingDeletion);
    const QStringList wallpapersAwaitingDeletion();

Q_SIGNALS:
    void countChanged();

protected Q_SLOTS:
    void showPreview(const KFileItem &item, const QPixmap &preview);
    void previewFailed(const KFileItem &item);
    void sizeFound(const QString &path, const QSize &s);
    void processPaths(const QStringList &paths);

protected:
    QPointer<Image> m_wallpaper;
    QString m_findToken;
    QList<KPackage::Package> m_packages;

private:
    QSize bestSize(const KPackage::Package &package) const;

    QSet<QString> m_removableWallpapers;
    QHash<QString, QSize> m_sizeCache;
    QHash<QPersistentModelIndex, QUrl> m_previewJobsUrls;
    KDirWatch m_dirwatch;
    QCache<QString, QPixmap> m_imageCache;

    int m_screenshotSize;
    QHash<QString, int> m_pendingDeletion;
};

class BackgroundFinder : public QThread
{
    Q_OBJECT

public:
    BackgroundFinder(Image *wallpaper, const QStringList &p);
    ~BackgroundFinder() override;

    QString token() const;

    static QStringList suffixes();
    static bool isAcceptableSuffix(const QString &suffix);

Q_SIGNALS:
    void backgroundsFound(const QStringList &paths, const QString &token);

protected:
    void run() override;

private:
    QStringList m_paths;
    QString m_token;

    static QMutex s_suffixMutex;
    static QStringList s_suffixes;
};

#endif // BACKGROUNDLISTMODEL_H
