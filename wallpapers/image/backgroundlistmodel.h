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
#include <QPixmap>
#include <QRunnable>
#include <QThread>

#include <KDirWatch>
#include <KFileItem>

#include <Plasma/PackageStructure>

class QEventLoop;
class KProgressDialog;

namespace Plasma
{
    class Package;
} // namespace Plasma

class Image;

class ImageSizeFinder : public QObject, public QRunnable
{
    Q_OBJECT
    public:
        ImageSizeFinder(const QString &path, QObject *parent = 0);
        void run();

    Q_SIGNALS:
        void sizeFound(const QString &path, const QSize &size);

    private:
        QString m_path;
};

class BackgroundListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum {
        AuthorRole = Qt::UserRole,
        ScreenshotRole,
        ResolutionRole,
        PathRole
    };
    static const int SCREENSHOT_SIZE = 96;
    static const int BLUR_INCREMENT = 9;
    static const int MARGIN = 6;

    BackgroundListModel(Image *listener, QObject *parent);
    virtual ~BackgroundListModel();

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Plasma::Package package(int index) const;

    void reload();
    void reload(const QStringList &selected);
    void addBackground(const QString &path);
    QModelIndex indexOf(const QString &path) const;
    virtual bool contains(const QString &bg) const;

protected Q_SLOTS:
    void removeBackground(const QString &path);
    void showPreview(const KFileItem &item, const QPixmap &preview);
    void previewFailed(const KFileItem &item);
    void sizeFound(const QString &path, const QSize &s);
    void backgroundsFound(const QStringList &paths, const QString &token);
    void processPaths(const QStringList &paths);

private:
    QSize bestSize(const Plasma::Package &package) const;

    QWeakPointer<Image> m_structureParent;
    QList<Plasma::Package> m_packages;
    QHash<QString, QSize> m_sizeCache;
    QHash<QString, QPixmap> m_previews;
    QHash<QUrl, QPersistentModelIndex> m_previewJobs;
    KDirWatch m_dirwatch;

    QString m_findToken;
    QPixmap m_previewUnavailablePix;
};

class BackgroundFinder : public QThread
{
    Q_OBJECT

public:
    BackgroundFinder(Image *structureParent, const QStringList &p);
    ~BackgroundFinder();

    QString token() const;

    static const QSet<QString> &suffixes();

Q_SIGNALS:
    void backgroundsFound(const QStringList &paths, const QString &token);

protected:
    void run();

private:
    QStringList m_paths;
    QString m_token;

    static QSet<QString> m_suffixes;
};

#endif // BACKGROUNDLISTMODEL_H
