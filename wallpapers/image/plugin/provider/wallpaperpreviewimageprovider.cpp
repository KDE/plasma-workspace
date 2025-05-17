/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "wallpaperpreviewimageprovider.h"
#include "finder/packagefinder.h"

#include <KPackage/Package>
#include <KPackage/PackageLoader>

#include <QPainter>

static QList<QUrl> extractImagesFromPackage(const QString &filePath, const QSize &targetSize)
{
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"), filePath);
    PackageFinder::findPreferredImageInPackage(package, targetSize);

    QList<QUrl> result;

    const QString preferred = package.filePath(QByteArrayLiteral("preferred"));
    if (!preferred.isEmpty()) {
        result.append(QUrl::fromLocalFile(preferred));
    }

    const QString preferredDark = package.filePath(QByteArrayLiteral("preferredDark"));
    if (!preferredDark.isEmpty()) {
        result.append(QUrl::fromLocalFile(preferredDark));
    }

    return result;
}

WallpaperPreviewImageResponse::WallpaperPreviewImageResponse(const QList<QUrl> &fileUrls, const QSize &requestedSize)
    : m_fileUrls(fileUrls)
{
    KFileItemList list;
    for (const QUrl &url : fileUrls) {
        list.append(KFileItem(url, QString(), 0));
    }

    const QStringList availablePlugins = KIO::PreviewJob::availablePlugins();
    m_previewJob = KIO::filePreview(list, requestedSize, &availablePlugins);
    m_previewJob->setIgnoreMaximumSize(true);

    connect(m_previewJob, &KIO::PreviewJob::gotPreview, this, &WallpaperPreviewImageResponse::onGotPreview);
    connect(m_previewJob, &KIO::PreviewJob::failed, this, &WallpaperPreviewImageResponse::onPreviewFailed);
}

WallpaperPreviewImageResponse::WallpaperPreviewImageResponse(const QUrl &fileUrl, const QSize &requestedSize)
    : WallpaperPreviewImageResponse(QList{fileUrl}, requestedSize)
{
}

QString WallpaperPreviewImageResponse::errorString() const
{
    return m_errorString;
}

QQuickTextureFactory *WallpaperPreviewImageResponse::textureFactory() const
{
    return QQuickTextureFactory::textureFactoryForImage(m_preview);
}

void WallpaperPreviewImageResponse::onGotPreview(const KFileItem &item, const QPixmap &pixmap)
{
    const int index = m_fileUrls.indexOf(item.url());
    if (index == -1) {
        m_errorString = QStringLiteral("received unexpected preview");
        Q_EMIT finished();
        return;
    }

    m_parts[index] = pixmap;
    if (m_parts.size() != m_fileUrls.size()) {
        return;
    }

    if (m_parts.size() == 1) {
        m_preview = m_parts[0].toImage(); // TODO: Extend KIO::PreviewJob API so it provides us QImages
    } else {
        m_preview = QImage(m_parts[0].size(), QImage::Format_ARGB32_Premultiplied);

        QPainter painter(&m_preview);
        for (const auto &[key, value] : std::as_const(m_parts).asKeyValueRange()) {
            const int start = (key * m_preview.width()) / m_parts.size();
            const int end = ((key + 1) * m_preview.width()) / m_parts.size();
            painter.setClipRect(QRect(start, 0, end - start, m_preview.height()));
            painter.drawPixmap(m_preview.rect(), value, value.rect());
        }
    }

    m_parts.clear();
    Q_EMIT finished();
}

void WallpaperPreviewImageResponse::onPreviewFailed(const KFileItem &item)
{
    m_errorString = QStringLiteral("failed to get preview for ") + item.url().toString();
    Q_EMIT finished();
}

WallpaperPreviewImageProvider::WallpaperPreviewImageProvider()
{
}

QQuickImageResponse *WallpaperPreviewImageProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    const QString packagePrefix = QStringLiteral("package=");
    const QString imagePrefix = QStringLiteral("image=");

    if (id.startsWith(packagePrefix)) {
        const auto components = extractImagesFromPackage(id.sliced(packagePrefix.size()), requestedSize);
        return new WallpaperPreviewImageResponse(components, requestedSize);
    } else if (id.startsWith(imagePrefix)) {
        return new WallpaperPreviewImageResponse(QUrl::fromLocalFile(id.sliced(imagePrefix.size())), requestedSize);
    } else {
        Q_UNREACHABLE();
    }
}

#include "moc_wallpaperpreviewimageprovider.cpp"
