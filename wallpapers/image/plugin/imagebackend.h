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
#include <QQmlParserStatus>
#include <QRect>
#include <QSize>
#include <QTimer>

#include <KPackage/Package>

#include "sortingmode.h"
#include "xmlslideshowupdatetimer.h"

class QFileDialog;
class QPalette;
class QQuickItem;
class QQuickWindow;

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
    /**
     * When the current wallpaper is a transition type wallpaper, disable
     * the corss-fade animation when changing wallpapers.
     */
    Q_PROPERTY(bool isTransition READ isTransition NOTIFY isTransitionChanged)

    Q_PROPERTY(QAbstractItemModel *wallpaperModel READ wallpaperModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel *slideFilterModel READ slideFilterModel CONSTANT)
    Q_PROPERTY(int slideTimer READ slideTimer WRITE setSlideTimer NOTIFY slideTimerChanged)
    Q_PROPERTY(QStringList slidePaths READ slidePaths WRITE setSlidePaths NOTIFY slidePathsChanged)
    Q_PROPERTY(QSize targetSize READ targetSize WRITE setTargetSize NOTIFY targetSizeChanged)
    Q_PROPERTY(QStringList uncheckedSlides READ uncheckedSlides WRITE setUncheckedSlides NOTIFY uncheckedSlidesChanged)

    /**
     * @return @c true if the image should span multiple screens, @c false otherwise
     * @see \WideImageProvider
     */
    Q_PROPERTY(bool spanScreens READ spanScreens WRITE setSpanScreens NOTIFY spanScreensChanged)

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
        Xml,
    };
    Q_ENUM(Provider)

    explicit ImageBackend(QObject *parent = nullptr);
    ~ImageBackend() override;

    QString image() const;
    void setImage(const QString &url);

    QUrl modelImage() const;

    bool isTransition() const;

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

    bool spanScreens() const;
    void setSpanScreens(bool span);

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
    void spanScreensChanged();
    void loadingChanged();

    void isTransitionChanged();

protected Q_SLOTS:
    void showAddSlidePathsDialog();
    void slotWallpaperBrowseCompleted();
    void slotUpdateXmlModelImage(const QPalette &palette);
    void slotPrepareForSleep(bool sleep);
    void startSlideshow();
    void addDirFromSelectionDialog();
    void backgroundsFound();

    void slotParentWindowChanged(QQuickWindow *window);
    void slotScreenPoolChanged();

protected:
    void setSingleImage();

private:
    SlideModel *slideshowModel();

    void toggleXmlSlideshow(bool enabled);

    bool m_ready = false;
    int m_delay = 10;
    QUrl m_image;
    QUrl m_modelImage;
    QSize m_targetSize;

    // Used by WideImageProvider
    QQuickWindow *m_targetWindow = nullptr;
    bool m_spanScreens = false;

    bool m_usedInConfig = true;

    RenderingMode m_mode = SingleImage;
    Provider m_providerType = Provider::Image;
    SortingMode::Mode m_slideshowMode = SortingMode::Random;
    bool m_slideshowFoldersFirst = false;

    KPackage::Package m_wallpaperPackage;
    QStringList m_slidePaths;
    QStringList m_uncheckedSlides;
    QTimer m_timer;
    XmlSlideshowUpdateTimer m_xmlTimer;
    int m_currentSlide = -1;
    ImageProxyModel *m_model = nullptr;
    SlideModel *m_slideshowModel = nullptr;
    SlideFilterModel *m_slideFilterModel;
    QFileDialog *m_dialog = nullptr;

    QMetaObject::Connection m_changeConnection;
    QMetaObject::Connection m_clockSkewdConnection;
    bool m_resumeConnection; // Is connected to the DBus signal
};
