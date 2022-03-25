/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

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

struct ImagePackage {
    explicit ImagePackage() = default;
    explicit ImagePackage(const QString &_path,
                          const QString &_darkPath = QString(),
                          const QString &_packagePath = QString(),
                          const QString &_name = QString(),
                          const QString &_author = QString());

    QString path;
    QString darkPath;
    QString packagePath;
    QString name;
    QString author;
};

using ImagePackageList = QVector<ImagePackage>;
Q_DECLARE_METATYPE(ImagePackageList)

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
        DarkScreenshotRole, /**< Dark-mode wallpaper preview */
        ResolutionRole,
        PathRole,
        DarkPathRole, /**< Dark-mode image path */
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
    void processImagePackages(const ImagePackageList &packages);

protected:
    QPointer<Image> m_wallpaper;
    QString m_findToken;
    ImagePackageList m_packages;

private:
    QSize bestSize(const QString &path) const;

    QSet<QString> m_removableWallpapers;
    QHash<QString, QSize> m_sizeCache;
    QHash<QUrl, QPersistentModelIndex> m_previewJobsUrls;
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

    /**
     * Reads the wallpaper paths from the wallpaper xml file.
     *
     * @see https://help.gnome.org/admin/system-admin-guide/stable/backgrounds-extra.html.en
     */
    static ImagePackageList parseXmlWallpaper(const QString &xmlPath);

Q_SIGNALS:
    void backgroundsFound(const ImagePackageList &packages, const QString &token);

protected:
    void run() override;

private:
    QStringList m_paths;
    QString m_token;

    static QMutex s_suffixMutex;
    static QStringList s_suffixes;
};
