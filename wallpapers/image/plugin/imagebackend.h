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
#include <QSize>
#include <QTimer>
#include <QUrl>

#include "sortingmode.h"

class QFileDialog;

class ImageProxyModel;
class SlideModel;
class SlideFilterModel;

class ImageBackend : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(bool usedInConfig MEMBER m_usedInConfig)
    Q_PROPERTY(RenderingMode renderingMode READ renderingMode WRITE setRenderingMode NOTIFY renderingModeChanged)
    Q_PROPERTY(SortingMode::Mode slideshowMode READ slideshowMode WRITE setSlideshowMode NOTIFY slideshowModeChanged)
    Q_PROPERTY(bool slideshowFoldersFirst READ slideshowFoldersFirst WRITE setSlideshowFoldersFirst NOTIFY slideshowFoldersFirstChanged)

    /**
     * Provides source url for \MediaProxy
     */
    Q_PROPERTY(QString image READ image WRITE setImage NOTIFY imageChanged)

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

    explicit ImageBackend(QObject *parent = nullptr);
    ~ImageBackend() override;

    QString image() const;
    void setImage(const QString &url);

    // this is for QML use
    Q_INVOKABLE void addSlidePath(const QUrl &url);
    Q_INVOKABLE void removeSlidePath(const QString &path);

    Q_INVOKABLE void showFileDialog();

    Q_INVOKABLE QString addUsersWallpaper(const QUrl &url);

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
    void renderingModeChanged();
    void slideshowModeChanged();
    void slideshowFoldersFirstChanged();
    void targetSizeChanged(const QSize &size);
    void slideTimerChanged();
    void slidePathsChanged();
    void uncheckedSlidesChanged();
    void loadingChanged();

    /**
     * Emitted when the user finishes adding images using the file dialog.
     */
    void wallpaperBrowseCompleted();

protected Q_SLOTS:
    void showAddSlidePathsDialog();
    void slotWallpaperBrowseCompleted();
    void startSlideshow();
    void addDirFromSelectionDialog();
    void backgroundsFound();

private:
    SlideModel *slideshowModel();

    bool m_ready = false;
    int m_delay = 10;
    QUrl m_image;
    QSize m_targetSize;

    bool m_usedInConfig = true;

    RenderingMode m_mode = SingleImage;
    SortingMode::Mode m_slideshowMode = SortingMode::Random;
    bool m_slideshowFoldersFirst = false;

    QStringList m_slidePaths;
    QStringList m_uncheckedSlides;
    QTimer m_timer;
    int m_currentSlide = -1;
    ImageProxyModel *m_model = nullptr;
    SlideModel *m_slideshowModel = nullptr;
    SlideFilterModel *m_slideFilterModel;
    QFileDialog *m_dialog = nullptr;
};
