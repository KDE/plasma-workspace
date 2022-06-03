/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "packageimageprovider.h"

#include <QImageReader>
#include <QUrlQuery>

#include <KPackage/PackageLoader>

#include "finder/packagefinder.h"
#include "utils.h"

class AsyncPackageImageResponseRunnable : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit AsyncPackageImageResponseRunnable(const QString &path, const QSize &requestedSize);

    /**
     * Read the image and resize it if the requested size is valid.
     */
    void run() override;

Q_SIGNALS:
    void done(const QImage &image);

private:
    QString m_path;
    QSize m_requestedSize;
};

class AsyncPackageImageResponse : public QQuickImageResponse
{
    Q_OBJECT

public:
    explicit AsyncPackageImageResponse(const QString &path, const QSize &requestedSize, QThreadPool *pool);

    QQuickTextureFactory *textureFactory() const override;

protected Q_SLOTS:
    void slotHandleDone(const QImage &image);

protected:
    QImage m_image;
};

AsyncPackageImageResponseRunnable::AsyncPackageImageResponseRunnable(const QString &path, const QSize &requestedSize)
    : m_path(path)
    , m_requestedSize(requestedSize)
{
}

void AsyncPackageImageResponseRunnable::run()
{
    const QUrlQuery urlQuery(QUrl(QStringLiteral("image://package/%1").arg(m_path)));
    const QString dir = urlQuery.queryItemValue(QStringLiteral("dir"));

    if (dir.isEmpty()) {
        Q_EMIT done(QImage());
        return;
    }

    const int spanScreens = urlQuery.queryItemValue(QStringLiteral("spanScreens")).toInt();
    const int desktopX = urlQuery.queryItemValue(QStringLiteral("desktopX")).toInt();
    const int desktopY = urlQuery.queryItemValue(QStringLiteral("desktopY")).toInt();
    const int desktopWidth = urlQuery.queryItemValue(QStringLiteral("desktopWidth")).toInt();
    const int desktopHeight = urlQuery.queryItemValue(QStringLiteral("desktopHeight")).toInt();

    const int totalRectX = urlQuery.queryItemValue(QStringLiteral("totalRectX")).toInt();
    const int totalRectY = urlQuery.queryItemValue(QStringLiteral("totalRectY")).toInt();
    const int totalRectWidth = urlQuery.queryItemValue(QStringLiteral("totalRectWidth")).toInt();
    const int totalRectHeight = urlQuery.queryItemValue(QStringLiteral("totalRectHeight")).toInt();

    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));
    package.setPath(dir);

    if (!package.isValid()) {
        Q_EMIT done(QImage());
        return;
    }

    QRect rect(totalRectX, totalRectY, totalRectWidth, totalRectHeight);

    if (spanScreens) {
        PackageFinder::findPreferredImageInPackage(package, rect.size());
    } else {
        PackageFinder::findPreferredImageInPackage(package, m_requestedSize);
    }

    QImage image(package.filePath("preferred"));

    if (image.isNull() || !m_requestedSize.isValid()) {
        Q_EMIT done(image);
        return;
    }

    if (spanScreens) {
        const QRect desktopRect(desktopX, desktopY, desktopWidth, desktopHeight);
        image = cropImage(image, rect, desktopRect, m_requestedSize);
    } else {
        image = image.scaled(m_requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    Q_EMIT done(image);
}

AsyncPackageImageResponse::AsyncPackageImageResponse(const QString &path, const QSize &requestedSize, QThreadPool *pool)
{
    auto runnable = new AsyncPackageImageResponseRunnable(path, requestedSize);
    connect(runnable, &AsyncPackageImageResponseRunnable::done, this, &AsyncPackageImageResponse::slotHandleDone);
    pool->start(runnable);
}

void AsyncPackageImageResponse::slotHandleDone(const QImage &image)
{
    m_image = image;
    Q_EMIT finished();
}

QQuickTextureFactory *AsyncPackageImageResponse::textureFactory() const
{
    return QQuickTextureFactory::textureFactoryForImage(m_image);
}

PackageImageProvider::PackageImageProvider()
{
}

QQuickImageResponse *PackageImageProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    AsyncPackageImageResponse *response = new AsyncPackageImageResponse(id, requestedSize, &m_pool);

    return response;
}

#include "packageimageprovider.moc"
