/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "previewimageprovider.h"

#include <QIcon>
#include <QImage>

#include <KFileItem>
#include <KIO/PreviewJob>

using namespace Qt::StringLiterals;

class AsyncPreviewImageResponse : public QQuickImageResponse
{
    Q_OBJECT

public:
    explicit AsyncPreviewImageResponse(const QString &path, const QSize &requestedSize);

    QQuickTextureFactory *textureFactory() const override;

protected:
    void saveImage(QImage &&image);
    QImage m_image;
};

AsyncPreviewImageResponse::AsyncPreviewImageResponse(const QString &path, const QSize &requestedSize)
{
    auto saveIcon = [this, requestedSize](const KFileItem &item) {
        saveImage(QIcon::fromTheme(item.currentMimeType().iconName()).pixmap(requestedSize.isValid() ? requestedSize : QSize(128, 128)).toImage());
    };

    const QUrl url = QUrl::fromUserInput(path);
    const KFileItem fileItem(url);
    if (!url.isValid() || fileItem.isSlow()) { // no remote files
        saveIcon(fileItem);
        return;
    }

    KIO::PreviewJob *job = KIO::filePreview(KFileItemList{fileItem}, requestedSize);
    job->setIgnoreMaximumSize(true);
    connect(job, &KIO::PreviewJob::gotPreview, this, [this](const KFileItem &, const QPixmap &preview) {
        saveImage(preview.toImage());
    });
    connect(job, &KIO::PreviewJob::failed, this, saveIcon);

    job->start();
}

void AsyncPreviewImageResponse::saveImage(QImage &&image)
{
    m_image = std::move(image);
    Q_EMIT finished();
}

QQuickTextureFactory *AsyncPreviewImageResponse::textureFactory() const
{
    return QQuickTextureFactory::textureFactoryForImage(m_image);
}

PreviewImageProvider::PreviewImageProvider() = default;

QQuickImageResponse *PreviewImageProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    return new AsyncPreviewImageResponse(id, requestedSize);
}

#include "previewimageprovider.moc"
