/***************************************************************************
 *   Copyright 2007 Paolo Capriotti <p.capriotti@gmail.com>                *
 *   Copyright 2008 by Petri Damsten <damu@iki.fi>                         *
 *   Copyright 2014 Sebastian Kügler <sebas@kde.org>                       *
 *   Copyright 2015 Kai Uwe Broulik <kde@privat.broulik.de>                *
 *   Copyright 2019 David Redondo <kde@david-redondo.de>                   *
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


#ifndef IMAGE_HEADER
#define IMAGE_HEADER


#include <QTimer>
#include <QPixmap>
#include <QStringList>
#include <QObject>
#include <QPersistentModelIndex>
#include <QDateTime>
#include <QPointer>
#include <QQmlParserStatus>

#include <KPackage/Package>


class QFileDialog;
class QQuickItem;

class KDirWatch;
class KJob;

namespace KNS3 {
    class DownloadDialog;
}

class BackgroundListModel;
class SlideModel;
class SlideFilterModel;

class Image : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(RenderingMode renderingMode READ renderingMode WRITE setRenderingMode NOTIFY renderingModeChanged)
    Q_PROPERTY(SlideshowMode slideshowMode READ slideshowMode WRITE setSlideshowMode NOTIFY slideshowModeChanged)
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
            SlideShow
        };
        Q_ENUM(RenderingMode)

        enum SlideshowMode {
            Random,
            Alphabetical,
            AlphabeticalReversed,
            Modified,
            ModifiedReversed
        };
        Q_ENUM(SlideshowMode)

        explicit Image(QObject* parent = nullptr);
        ~Image() override;

        QUrl wallpaperPath() const;

        //this is for QML use
        Q_INVOKABLE void addUrl(const QString &url);
        Q_INVOKABLE void addUrls(const QStringList &urls);

        Q_INVOKABLE void addSlidePath(const QString &path);
        Q_INVOKABLE void removeSlidePath(const QString &path);
        Q_INVOKABLE void openFolder(const QString& path);

        Q_INVOKABLE void showFileDialog();

        Q_INVOKABLE void addUsersWallpaper(const QString &file);
        Q_INVOKABLE void commitDeletion();

        Q_INVOKABLE void toggleSlide(const QString &path, bool checked);

        RenderingMode renderingMode() const;
        void setRenderingMode(RenderingMode mode);

        SlideshowMode slideshowMode() const;
        void setSlideshowMode(SlideshowMode mode);

        QSize targetSize() const;
        void setTargetSize(const QSize &size);

        KPackage::Package *package();

        QAbstractItemModel* wallpaperModel();
        QAbstractItemModel* slideFilterModel();

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
        /**
         * Open the current slide in the default image application
         */
        void openSlide();
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

        KPackage::Package m_wallpaperPackage;
        QStringList m_slidePaths;
        QStringList m_uncheckedSlides;
        QTimer m_timer;
        int m_currentSlide;
        BackgroundListModel *m_model;
        SlideModel* m_slideshowModel;
        SlideFilterModel* m_slideFilterModel;
        QFileDialog *m_dialog;
        QString m_img;
        QDateTime m_previousModified;
        QPointer<KNS3::DownloadDialog> m_newStuffDialog;
        QString m_findToken;
};

#endif
