/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "packageimageprovider.h"

#include <QGuiApplication>
#include <QImageReader>
#include <QPainter>
#include <QPalette>
#include <QUrlQuery>

#include <KPackage/PackageLoader>

#include "debug.h"
#include "finder/imagepackage.h"

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
    void parseStaticWallpaper(const KPackage::ImagePackage &imagePackage);
    void parseDynamicWallpaper(const KPackage::ImagePackage &imagePackage);

    QImage blendImages(const QImage &from, const QImage &to, double toOpacity) const;

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

    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));
    package.setPath(dir);

    if (!package.isValid()) {
        Q_EMIT done(QImage());
        return;
    }

    const KPackage::ImagePackage imagePackage(package, m_requestedSize);
    if (imagePackage.dynamicType() == DynamicType::None) {
        parseStaticWallpaper(imagePackage);
    } else {
        parseDynamicWallpaper(imagePackage);
    }
}

void AsyncPackageImageResponseRunnable::parseStaticWallpaper(const KPackage::ImagePackage &imagePackage)
{
    QString path = imagePackage.preferred().toLocalFile();
    // 192 is from kcm_colors
    if (qGray(qGuiApp->palette().window().color().rgb()) < 192) {
        QString darkPath = imagePackage.preferredDark().toLocalFile();

        if (!darkPath.isEmpty()) {
            path = darkPath;
        }
    }

    QImage image(path);

    if (!image.isNull() && m_requestedSize.isValid()) {
        image = image.scaled(m_requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    Q_EMIT done(image);
}

void AsyncPackageImageResponseRunnable::parseDynamicWallpaper(const KPackage::ImagePackage &imagePackage)
{
    // Caculate cycle length
    const int index = imagePackage.indexAndIntervalAtDateTime(QDateTime::currentDateTime()).first;
    if (index < 0) {
        Q_EMIT done(QImage());
        return;
    }

    const auto &item = imagePackage.dynamicMetadataAtIndex(index);
    QImage image(item.filename);

    if (imagePackage.dynamicMetadataSize() == 1) {
        Q_EMIT done(image);
        return;
    }

    switch (item.type) {
    case DynamicMetadataItem::Static: {
        break;
    }

    case DynamicMetadataItem::Transition: {
        QImage from;
        if (index == 0) {
            // Use the last image
            from.load(imagePackage.dynamicMetadataAtIndex(imagePackage.dynamicMetadataSize() - 1).filename);
        } else {
            from.load(imagePackage.dynamicMetadataAtIndex(index - 1).filename);
        }

        const auto timeInfoListPair = imagePackage.dynamicTimeList();
        const quint64 modTime = imagePackage.startTime().secsTo(QDateTime::currentDateTime()) % timeInfoListPair.second;

        const auto currentTimeInfo = timeInfoListPair.first.at(index);
        const auto nextTimeInfo = timeInfoListPair.first.at(index + 1);
        const double opacity =
            1.0 - (nextTimeInfo.accumulatedTime - modTime) / static_cast<double>(nextTimeInfo.accumulatedTime - currentTimeInfo.accumulatedTime);

        image = blendImages(from, image, opacity);
    }
    }

    if (!image.isNull() && m_requestedSize.isValid()) {
        image = image.scaled(m_requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    Q_EMIT done(image);
}

QImage AsyncPackageImageResponseRunnable::blendImages(const QImage &from, const QImage &to, double toOpacity) const
{
    if (from.isNull() || toOpacity < 0 || toOpacity > 1) {
        qCWarning(IMAGEWALLPAPER) << "opacity" << toOpacity << "is not a valid number.";
        return to;
    }

    QImage base = from.convertToFormat(QImage::Format_ARGB32);

    auto p = std::make_unique<QPainter>();
    if (!p->begin(&base)) {
        qCWarning(IMAGEWALLPAPER) << "failed to initialize QPainter! Dynamic wallpaper may not work as expected.";
        return to;
    }

    qCDebug(IMAGEWALLPAPER) << "new opacity:" << toOpacity;

    QImage overlay = to.convertToFormat(QImage::Format_ARGB32);
    p->setOpacity(toOpacity);
    p->drawImage(QRect(0, 0, base.width(), base.height()), overlay);
    p->end();

    return base;
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
