/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "wideimageprovider.h"

#include <QGuiApplication> // For screens
#include <QImageReader>
#include <QScreen>
#include <QUrlQuery>

#include "debug.h"
#include "utils.h"

class AsyncWideImageResponseRunnable : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit AsyncWideImageResponseRunnable(const QString &path, const QSize &requestedSize);

    /**
     * Read the image and crop it if the requested size is valid.
     */
    void run() override;

Q_SIGNALS:
    void done(const QImage &image);

private:
    QString m_path;
    QSize m_requestedSize;
};

class AsyncWideImageResponse : public QQuickImageResponse
{
    Q_OBJECT

public:
    explicit AsyncWideImageResponse(const QString &path, const QSize &requestedSize, QThreadPool *pool);

    QQuickTextureFactory *textureFactory() const override;

protected Q_SLOTS:
    void slotHandleDone(const QImage &image);

protected:
    QImage m_image;
};

AsyncWideImageResponseRunnable::AsyncWideImageResponseRunnable(const QString &path, const QSize &requestedSize)
    : m_path(path)
    , m_requestedSize(requestedSize)
{
}

void AsyncWideImageResponseRunnable::run()
{
    const QUrlQuery urlQuery(QUrl(QStringLiteral("image://wideimage/%1").arg(m_path)));

    const QString path = urlQuery.queryItemValue(QStringLiteral("path"));

    const int desktopX = urlQuery.queryItemValue(QStringLiteral("desktopX")).toInt();
    const int desktopY = urlQuery.queryItemValue(QStringLiteral("desktopY")).toInt();
    const int desktopWidth = urlQuery.queryItemValue(QStringLiteral("desktopWidth")).toInt();
    const int desktopHeight = urlQuery.queryItemValue(QStringLiteral("desktopHeight")).toInt();

    const int totalRectX = urlQuery.queryItemValue(QStringLiteral("totalRectX")).toInt();
    const int totalRectY = urlQuery.queryItemValue(QStringLiteral("totalRectY")).toInt();
    const int totalRectWidth = urlQuery.queryItemValue(QStringLiteral("totalRectWidth")).toInt();
    const int totalRectHeight = urlQuery.queryItemValue(QStringLiteral("totalRectHeight")).toInt();

    if (path.isEmpty() || !m_requestedSize.isValid()) {
        Q_EMIT done(QImage());
        return;
    }

    QImage image(path);

    if (image.isNull()) {
        Q_EMIT done(image);
        return;
    }

    QRect rect(totalRectX, totalRectY, totalRectWidth, totalRectHeight);
    qCDebug(IMAGEWALLPAPER) << "Requested size:" << m_requestedSize << "Total geometry:" << rect;

    Q_ASSERT(rect.width() >= m_requestedSize.width());
    Q_ASSERT(rect.height() >= m_requestedSize.height());

    const QRect desktopRect(desktopX, desktopY, desktopWidth, desktopHeight);
    image = cropImage(image, rect, desktopRect, m_requestedSize);

    Q_EMIT done(image);
}

AsyncWideImageResponse::AsyncWideImageResponse(const QString &path, const QSize &requestedSize, QThreadPool *pool)
{
    auto runnable = new AsyncWideImageResponseRunnable(path, requestedSize);
    connect(runnable, &AsyncWideImageResponseRunnable::done, this, &AsyncWideImageResponse::slotHandleDone);
    pool->start(runnable);
}

void AsyncWideImageResponse::slotHandleDone(const QImage &image)
{
    m_image = image;
    Q_EMIT finished();
}

QQuickTextureFactory *AsyncWideImageResponse::textureFactory() const
{
    return QQuickTextureFactory::textureFactoryForImage(m_image);
}

WideImageProvider::WideImageProvider()
{
}

QQuickImageResponse *WideImageProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    AsyncWideImageResponse *response = new AsyncWideImageResponse(id, requestedSize, &m_pool);

    return response;
}

#include "wideimageprovider.moc"
