/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>
    SPDX-FileCopyrightText: 2008 Petri Damsten <damu@iki.fi>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2015 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QDateTime>
#include <QObject>
#include <QPersistentModelIndex>
#include <QPixmap>
#include <QPointer>
#include <QQmlParserStatus>
#include <QStringList>
#include <QTimer>

#include <KPackage/Package>

class QFileDialog;
class QQuickItem;

class KDirWatch;
class KJob;
class BackgroundListModel;
class SlideModel;
class SlideFilterModel;

class Image : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(RenderingMode renderingMode READ renderingMode WRITE setRenderingMode NOTIFY renderingModeChanged)
    Q_PROPERTY(SlideshowMode slideshowMode READ slideshowMode WRITE setSlideshowMode NOTIFY slideshowModeChanged)
    Q_PROPERTY(bool slideshowFoldersFirst READ slideshowFoldersFirst WRITE setSlideshowFoldersFirst NOTIFY slideshowFoldersFirstChanged)
    Q_PROPERTY(QUrl wallpaperPath READ wallpaperPath NOTIFY wallpaperPathChanged)
    Q_PROPERTY(QAbstractItemModel *wallpaperModel READ wallpaperModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel *slideFilterModel READ slideFilterModel CONSTANT)
    Q_PROPERTY(int slideTimer READ slideTimer WRITE setSlideTimer NOTIFY slideTimerChanged)
    Q_PROPERTY(QStringList usersWallpapers READ usersWallpapers WRITE setUsersWallpapers NOTIFY usersWallpapersChanged)
    Q_PROPERTY(QStringList slidePaths READ slidePaths WRITE setSlidePaths NOTIFY slidePathsChanged)
    Q_PROPERTY(QSize targetSize READ targetSize WRITE setTargetSize NOTIFY targetSizeChanged)
    Q_PROPERTY(QString photosPath READ photosPath CONSTANT)
    Q_PROPERTY(QStringList uncheckedSlides READ uncheckedSlides WRITE setUncheckedSlides NOTIFY uncheckedSlidesChanged)

public:
    enum RenderingMode {
        SingleImage,
        SlideShow,
    };
    Q_ENUM(RenderingMode)

    enum SlideshowMode {
        Random,
        Alphabetical,
        AlphabeticalReversed,
        Modified,
        ModifiedReversed,
    };
    Q_ENUM(SlideshowMode)

    explicit Image(QObject* parent = nullptr);
    ~Image() override;

    QUrl wallpaperPath() const;

    // this is for QML use
    Q_INVOKABLE void addUrl(const QString &url);
    Q_INVOKABLE void addUrls(const QStringList &urls);

    Q_INVOKABLE void addSlidePath(const QString &path);
    Q_INVOKABLE void removeSlidePath(const QString &path);
    Q_INVOKABLE void openFolder(const QString &path);

    Q_INVOKABLE void showFileDialog();

    Q_INVOKABLE void addUsersWallpaper(const QString &file);
    Q_INVOKABLE void commitDeletion();

    Q_INVOKABLE void toggleSlide(const QString &path, bool checked);

    RenderingMode renderingMode() const;
    void setRenderingMode(RenderingMode mode);

    SlideshowMode slideshowMode() const;
    void setSlideshowMode(SlideshowMode slideshowMode);

    bool slideshowFoldersFirst() const;
    void setSlideshowFoldersFirst(bool slideshowFoldersFirst);

    QSize targetSize() const;
    void setTargetSize(const QSize &size);

    KPackage::Package *package();

    QAbstractItemModel *wallpaperModel();
    QAbstractItemModel *slideFilterModel();

    int slideTimer() const;
    void setSlideTimer(int time);

    QStringList usersWallpapers() const;
    void setUsersWallpapers(const QStringList &usersWallpapers);

    QStringList slidePaths() const;
    void setSlidePaths(const QStringList &slidePaths);

    void findPreferedImageInPackage(KPackage::Package &package);
    QString findPreferedImage(const QStringList &images);

    void classBegin() override;
    void componentComplete() override;

    QString photosPath() const;

    QStringList uncheckedSlides() const;
    void setUncheckedSlides(const QStringList &uncheckedSlides);

public Q_SLOTS:
    void nextSlide();
    void removeWallpaper(QString name);

Q_SIGNALS:
    void settingsChanged(bool);
    void wallpaperPathChanged();
    void renderingModeChanged();
    void slideshowModeChanged();
    void slideshowFoldersFirstChanged();
    void targetSizeChanged();
    void slideTimerChanged();
    void usersWallpapersChanged();
    void slidePathsChanged();
    void resizeMethodChanged();
    void customWallpaperPicked(const QString &path);
    void uncheckedSlidesChanged();

protected Q_SLOTS:
    void showAddSlidePathsDialog();
    void wallpaperBrowseCompleted();
    void startSlideshow();
    void fileDialogFinished();
    void addUrl(const QUrl &url, bool setAsCurrent);
    void addUrls(const QList<QUrl> &urls);
    void setWallpaper(const QString &path);
    void setWallpaperRetrieved(KJob *job);
    void addWallpaperRetrieved(KJob *job);
    void newStuffFinished();
    void updateDirWatch(const QStringList &newDirs);
    void addDirFromSelectionDialog();
    void pathCreated(const QString &path);
    void pathDeleted(const QString &path);
    void pathDirty(const QString &path);
    void backgroundsFound();

protected:
    void syncWallpaperPackage();
    void setSingleImage();
    void useSingleImageDefaults();

private:
    bool m_ready;
    int m_delay;
    QStringList m_dirs;
    QString m_wallpaper;
    QString m_wallpaperPath;
    QStringList m_usersWallpapers;
    KDirWatch *m_dirWatch;
    bool m_scanDirty;
    QSize m_targetSize;

    RenderingMode m_mode;
    SlideshowMode m_slideshowMode;
    bool m_slideshowFoldersFirst;

    KPackage::Package m_wallpaperPackage;
    QStringList m_slidePaths;
    QStringList m_uncheckedSlides;
    QTimer m_timer;
    int m_currentSlide;
    BackgroundListModel *m_model;
    SlideModel *m_slideshowModel;
    SlideFilterModel *m_slideFilterModel;
    QFileDialog *m_dialog;
    QString m_img;
    QDateTime m_previousModified;
    QString m_findToken;
};
