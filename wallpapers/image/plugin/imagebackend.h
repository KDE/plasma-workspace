/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>
    SPDX-FileCopyrightText: 2008 Petri Damsten <damu@iki.fi>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2015 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QAbstractItemModel>
#include <QPalette>
#include <QQmlParserStatus>
#include <QSize>
#include <QTimer>

#include <KPackage/Package>

#include "sortingmode.h"

class QFileDialog;
class QQuickItem;

class KDirWatch;
class KJob;
class ImageProxyModel;
class SlideModel;
class SlideFilterModel;

class ImageBackend : public QObject, public QQmlParserStatus, public SortingMode
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(bool usedInConfig MEMBER m_usedInConfig)
    Q_PROPERTY(RenderingMode renderingMode READ renderingMode WRITE setRenderingMode NOTIFY renderingModeChanged)
    Q_PROPERTY(SortingMode::Mode slideshowMode READ slideshowMode WRITE setSlideshowMode NOTIFY slideshowModeChanged)
    Q_PROPERTY(bool slideshowFoldersFirst READ slideshowFoldersFirst WRITE setSlideshowFoldersFirst NOTIFY slideshowFoldersFirstChanged)
    /**
     * Package path from the saved configuration, can be an image file, a url with
     * "image://" scheme or a folder (KPackage).
     */
    Q_PROPERTY(QString image READ image WRITE setImage NOTIFY imageChanged)
    /**
     * The real path of the image
     * e.g. /home/kde/Pictures/image.png
     *      image://package/get? (KPackage)
     */
    Q_PROPERTY(QUrl modelImage READ modelImage NOTIFY modelImageChanged)
    Q_PROPERTY(QAbstractItemModel *wallpaperModel READ wallpaperModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel *slideFilterModel READ slideFilterModel CONSTANT)
    Q_PROPERTY(int slideTimer READ slideTimer WRITE setSlideTimer NOTIFY slideTimerChanged)
    Q_PROPERTY(QStringList slidePaths READ slidePaths WRITE setSlidePaths NOTIFY slidePathsChanged)
    Q_PROPERTY(QSize targetSize READ targetSize WRITE setTargetSize NOTIFY targetSizeChanged)
    Q_PROPERTY(QStringList uncheckedSlides READ uncheckedSlides WRITE setUncheckedSlides NOTIFY uncheckedSlidesChanged)

    /**
     * @return @c true if the image list is loaded, @c false otherwise
     */
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

public:
    enum RenderingMode {
        SingleImage,
        SlideShow,
    };
    Q_ENUM(RenderingMode)

    enum class Provider {
        Image,
        Package,
    };
    Q_ENUM(Provider)

    explicit ImageBackend(QObject *parent = nullptr);
    ~ImageBackend() override;

    QString image() const;
    void setImage(const QString &url);

    QUrl modelImage() const;

    // this is for QML use
    Q_INVOKABLE void addSlidePath(const QUrl &url);
    Q_INVOKABLE void removeSlidePath(const QString &path);
    Q_INVOKABLE void openFolder(const QString &path);
    Q_INVOKABLE void openModelImage() const;

    Q_INVOKABLE void showFileDialog();

    Q_INVOKABLE QString addUsersWallpaper(const QUrl &url);

    Q_INVOKABLE void useSingleImageDefaults();

    RenderingMode renderingMode() const;
    void setRenderingMode(RenderingMode mode);

    SortingMode::Mode slideshowMode() const;
    void setSlideshowMode(SortingMode::Mode slideshowMode);

    bool slideshowFoldersFirst() const;
    void setSlideshowFoldersFirst(bool slideshowFoldersFirst);

    QSize targetSize() const;
    void setTargetSize(const QSize &size);

    QAbstractItemModel *wallpaperModel();
    QAbstractItemModel *slideFilterModel();

    int slideTimer() const;
    void setSlideTimer(int time);

    QStringList slidePaths() const;
    void setSlidePaths(const QStringList &slidePaths);

    void classBegin() override;
    void componentComplete() override;

    QStringList uncheckedSlides() const;
    void setUncheckedSlides(const QStringList &uncheckedSlides);

    bool loading() const;

public Q_SLOTS:
    void nextSlide();
    void slotSlideModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);

Q_SIGNALS:
    void settingsChanged();
    void imageChanged();
    void modelImageChanged();
    void renderingModeChanged();
    void slideshowModeChanged();
    void slideshowFoldersFirstChanged();
    void targetSizeChanged(const QSize &size);
    void slideTimerChanged();
    void usersWallpapersChanged();
    void slidePathsChanged();
    void resizeMethodChanged();
    void customWallpaperPicked(const QString &path);
    void uncheckedSlidesChanged();
    void loadingChanged();

    /**
     * Emitted when system color scheme changes. The frontend is required to
     * reload the wallpaper even if the image path is not changed.
     */
    void colorSchemeChanged();

protected Q_SLOTS:
    void showAddSlidePathsDialog();
    void slotWallpaperBrowseCompleted();
    void startSlideshow();
    void addDirFromSelectionDialog();
    void backgroundsFound();

    /**
     * Switches to dark-colored wallpaper if available when system color
     * scheme is dark.
     *
     * @since 5.26
     */
    void slotSystemPaletteChanged(const QPalette &palette);

protected:
    void setSingleImage();

private:
    SlideModel *slideshowModel();

    inline bool isDarkColorScheme(const QPalette &palette = {}) const noexcept;

    bool m_ready = false;
    int m_delay = 10;
    QUrl m_image;
    QUrl m_modelImage;
    QSize m_targetSize;

    bool m_usedInConfig = true;

    RenderingMode m_mode = SingleImage;
    Provider m_providerType = Provider::Image;
    SortingMode::Mode m_slideshowMode = SortingMode::Random;
    bool m_slideshowFoldersFirst = false;

    KPackage::Package m_wallpaperPackage;
    QStringList m_slidePaths;
    QStringList m_uncheckedSlides;
    QTimer m_timer;
    int m_currentSlide = -1;
    ImageProxyModel *m_model = nullptr;
    SlideModel *m_slideshowModel = nullptr;
    SlideFilterModel *m_slideFilterModel;
    QFileDialog *m_dialog = nullptr;

    bool m_isDarkColorScheme;
};
