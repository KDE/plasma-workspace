/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef MEDIAPROXY_H
#define MEDIAPROXY_H

#include <QObject>
#include <QPalette>
#include <QQmlParserStatus>
#include <QSize>
#include <QUrl>

#include "../finder/imagepackage.h"
#include "../provider/providertype.h"
#include "backgroundtype.h"

class DynamicWallpaperUpdateTimer;

/**
 * A proxy class that converts a provider url to a real resource url.
 */
class MediaProxy : public QObject, public QQmlParserStatus, public Provider
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    /**
     * Package path from the saved configuration, can be an image file, a url with
     * "image://" scheme or a folder (KPackage).
     */
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)

    /**
     * The real path of the image
     * e.g. /home/kde/Pictures/image.png
     *      image://package/get? (KPackage)
     */
    Q_PROPERTY(QUrl modelImage READ modelImage NOTIFY modelImageChanged)

    /**
     * Type of the current wallpaper
     */
    Q_PROPERTY(BackgroundType::Type backgroundType MEMBER m_backgroundType NOTIFY backgroundTypeChanged)

    Q_PROPERTY(QSize targetSize READ targetSize WRITE setTargetSize NOTIFY targetSizeChanged)

public:
    explicit MediaProxy(QObject *parent = nullptr);

    void classBegin() override;
    void componentComplete() override;

    QString source() const;
    void setSource(const QString &url);

    QUrl modelImage() const;

    QSize targetSize() const;
    void setTargetSize(const QSize &size);

    Provider::Type providerType() const;

    Q_INVOKABLE void openModelImage();

    Q_INVOKABLE void useSingleImageDefaults();

    static QUrl formatUrl(const QUrl &url);

Q_SIGNALS:
    void sourceChanged();
    void modelImageChanged();

    /**
     * Emitted when the type of the current wallpaper changes.
     */
    void backgroundTypeChanged();

    void targetSizeChanged(const QSize &size);

    /**
     * Emitted when system color scheme changes. The frontend is required to
     * reload the wallpaper even if the image path is not changed.
     */
    void colorSchemeChanged();

private Q_SLOTS:
    /**
     * Switches to dark-colored wallpaper if available when system color
     * scheme is dark.
     *
     * @since 5.26
     */
    void slotSystemPaletteChanged(const QPalette &palette);

    /**
     * Updates model image for dynamic wallpaper.
     *
     * @since 5.26
     */
    void slotUpdateDynamicWallpaper();

private:
    inline bool isDarkColorScheme(const QPalette &palette = {}) const noexcept;

    void determineBackgroundType();
    void determineProviderType();
    void updateDynamicWallpaper();
    void updateWallpaper();

    QUrl findPreferredImageInPackage();
    void updateModelImage(bool doesBlockSignal = false);
    void updateModelImageWithoutSignal();

    bool m_ready = false;

    QUrl m_source;
    QUrl m_formattedSource;
    QUrl m_modelImage;
    BackgroundType::Type m_backgroundType = BackgroundType::Type::Unknown;
    Provider::Type m_providerType = Provider::Type::Unknown;

    QSize m_targetSize;

    bool m_isDarkColorScheme;

    // For dynamic wallpaper
    std::unique_ptr<KPackage::ImagePackage> m_imagePackage = nullptr;
    DynamicType::Type m_dynamicType = DynamicType::None;
    DynamicWallpaperUpdateTimer *m_dynamicTimer = nullptr;
};

#endif // MEDIAPROXY_H
