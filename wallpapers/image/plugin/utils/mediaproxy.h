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

#include "../provider/providertype.h"
#include "backgroundtype.h"
#include "xmlslideshowupdatetimer.h"

class QQuickWindow;

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

    /**
     * @return @c true if the image should span multiple screens, @c false otherwise
     */
    Q_PROPERTY(bool spanScreens READ spanScreens WRITE setSpanScreens NOTIFY spanScreensChanged)

    /**
     * The desktop window where the wallpaper is located
     */
    Q_PROPERTY(QQuickWindow *targetWindow READ targetWindow WRITE setTargetWindow NOTIFY targetWindowChanged)

    /**
     * The actual display area of the wallpaper
     */
    Q_PROPERTY(QRect targetRect MEMBER m_targetRect NOTIFY targetRectChanged)

public:
    explicit MediaProxy(QObject *parent = nullptr);
    ~MediaProxy();

    void classBegin() override;
    void componentComplete() override;

    QString source() const;
    void setSource(const QString &url);

    QUrl modelImage() const;

    QSize targetSize() const;
    void setTargetSize(const QSize &size);

    bool spanScreens() const;
    void setSpanScreens(bool span);

    QQuickWindow *targetWindow() const;
    void setTargetWindow(QQuickWindow *window);

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
    void spanScreensChanged();
    void targetWindowChanged();

    /**
     * Emitted when the actual display area in the wallpaper changes.
     */
    void targetRectChanged();

    /**
     * Emitted when system color scheme changes. The frontend is required to
     * reload the wallpaper even if the image path is not changed.
     */
    void colorSchemeChanged();

    /**
     * Emitted when the system clock is changed by the user.
     */
    void clockSkewed();

private Q_SLOTS:
    /**
     * Switches to dark-colored wallpaper if available when system color
     * scheme is dark.
     *
     * @since 5.26
     */
    void slotSystemPaletteChanged(const QPalette &palette);

    /**
     * Updates image for XML slideshow.
     */
    void slotUpdateXmlModelImage();

    /**
     * Updates image after resume from sleep.
     */
    void slotPrepareForSleep(bool sleep);

    /**
     * Updates the geometry of the wallpaper on each screen when the geometry of any
     * desktop window changes, or when the wallpaper settings changes.
     */
    void slotDesktopPoolChanged(QQuickWindow *sourceWindow);

private:
    inline bool isDarkColorScheme(const QPalette &palette = {}) const noexcept;

    void determineBackgroundType();
    void determineProviderType();

    QUrl findPreferredImageInPackage();
    QUrl findPreferredImageInXml();
    void updateModelImage(bool doesBlockSignal = false);
    void updateModelImageWithoutSignal();

    void toggleXmlSlideshow(bool enabled);

    void enableSpanScreen();
    void disableSpanScreen();

    bool m_ready = false;

    QUrl m_source;
    QUrl m_formattedSource;
    QUrl m_modelImage;
    BackgroundType::Type m_backgroundType = BackgroundType::Type::Unknown;
    Provider::Type m_providerType = Provider::Type::Unknown;

    QSize m_targetSize;

    bool m_isDarkColorScheme;

    // Used by WideImageProvider
    QQuickWindow *m_targetWindow = nullptr;
    bool m_spanScreens = false;
    QRect m_targetRect;

    // Used by XmlImageProvider
    XmlSlideshowUpdateTimer m_xmlTimer;
    bool m_resumeConnection = false; // Is connected to the DBus signal
};

#endif // MEDIAPROXY_H
