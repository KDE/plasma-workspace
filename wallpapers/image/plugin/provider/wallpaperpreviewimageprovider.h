/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KIO/PreviewJob>

#include <QQuickAsyncImageProvider>

class WallpaperPreviewImageResponse : public QQuickImageResponse
{
    Q_OBJECT

public:
    WallpaperPreviewImageResponse(const QList<QUrl> &fileUrls, const QSize &requestedSize);
    WallpaperPreviewImageResponse(const QUrl &fileUrl, const QSize &requestedSize);

    QString errorString() const override;
    QQuickTextureFactory *textureFactory() const override;

private Q_SLOTS:
    void onPreviewGenerated(const KFileItem &item, const QImage &preview);
    void onPreviewFailed(const KFileItem &item);

private:
    KIO::PreviewJob *m_previewJob = nullptr;
    QImage m_preview;
    QList<QUrl> m_fileUrls;
    QMap<int, QImage> m_parts;
    QString m_errorString;
};

class WallpaperPreviewImageProvider : public QQuickAsyncImageProvider
{
public:
    explicit WallpaperPreviewImageProvider();

    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;
};
