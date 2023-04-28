/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <QPalette>
#include <QQmlParserStatus>
#include <QSize>
#include <QUrl>

#include "../provider/providertype.h"
#include "backgroundtype.h"

namespace KPackage
{
class Package;
}

/**
 * A proxy class that converts a provider url to a real resource url.
 */
class MediaProxy : public QObject, public QQmlParserStatus
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
     * Type of the current background provider
     */
    Q_PROPERTY(Provider::Type providerType MEMBER m_providerType NOTIFY providerTypeChanged)

    /**
     * Type of the current wallpaper
     */
    Q_PROPERTY(BackgroundType::Type backgroundType MEMBER m_backgroundType NOTIFY backgroundTypeChanged)

    Q_PROPERTY(QSize targetSize READ targetSize WRITE setTargetSize NOTIFY targetSizeChanged)

    Q_PROPERTY(QColor customColor MEMBER m_customColor NOTIFY customColorChanged)

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

Q_SIGNALS:
    void sourceChanged();
    void modelImageChanged();

    /**
     * Emitted when the type of the current wallpaper changes.
     */
    void backgroundTypeChanged();

    void targetSizeChanged(const QSize &size);

    /**
     * Emitted when the target size changes while the provider type is single image
     * or the current wallpaper is animated.
     * The frontend is required to force update the wallpaper.
     */
    void actualSizeChanged();

    /**
     * Emitted when system color scheme changes. The frontend is required to
     * reload the wallpaper even if the image path is not changed.
     */
    void colorSchemeChanged();

    /**
     * Emitted when the type of the background provider changes.
     * @see determineProviderType
     */
    void providerTypeChanged();

    /**
     * The wallpaper package has specified a custom accent color, or
     * @c transparent if no/invalid color is specified.
     */
    void customColorChanged();

private Q_SLOTS:
    /**
     * Switches to dark-colored wallpaper if available when system color
     * scheme is dark.
     *
     * @since 5.26
     */
    void slotSystemPaletteChanged(const QPalette &palette);

private:
    static inline bool isDarkColorScheme(const QPalette &palette = {});

    /**
     * Reads accent color from wallpaper metadata. The value is stored under key
     * @c X-KDE-PlasmaImageWallpaper-AccentColor in metadata.json.
     *
     * @param package wallpaper package
     * @return custom accent color from the wallpaper package
     * @since 5.27
     */
    static QColor getAccentColorFromMetaData(const KPackage::Package &package);

    void determineBackgroundType(KPackage::Package &package);
    void determineProviderType();

    QUrl findPreferredImageInPackage(KPackage::Package &package);
    void updateModelImage(KPackage::Package &package, bool doesBlockSignal = false);
    void updateModelImageWithoutSignal(KPackage::Package &package);

    bool m_ready = false;

    QUrl m_source;
    bool m_isDefaultSource = false;
    QUrl m_modelImage;
    BackgroundType::Type m_backgroundType = BackgroundType::Type::Unknown;
    Provider::Type m_providerType = Provider::Type::Unknown;

    QSize m_targetSize;
    QColor m_customColor = Qt::transparent;

    bool m_isDarkColorScheme;

    friend class ImageFrontendTest;
};
