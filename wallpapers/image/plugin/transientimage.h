/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QImage>
#include <QQuickItem>
#include <QRunnable>
#include <QSGTexture>
#include <QSGTextureProvider>

struct TransientImageOptions;
class TransientImageReader;

class TransientImageNode : public QSGNode
{
public:
    explicit TransientImageNode(QQuickWindow *window);

    void setTargetRect(const QRectF &rect);
    void setSourceRect(const QRectF &rect);
    void setTexture(std::shared_ptr<QSGTexture> texture);
    void setFiltering(QSGTexture::Filtering filtering);

private:
    std::shared_ptr<QSGTexture> m_texture;
    std::unique_ptr<QSGImageNode> m_node;
};

class TransientImageTextureProvider : public QSGTextureProvider
{
    Q_OBJECT

public:
    TransientImageTextureProvider();

    QSGTexture *texture() const override;
    void setTexture(std::shared_ptr<QSGTexture> texture);

private:
    std::shared_ptr<QSGTexture> m_texture;
};

class TransientImageCleanupJob : public QRunnable
{
public:
    TransientImageCleanupJob(std::shared_ptr<QSGTexture> &&texture, std::unique_ptr<TransientImageTextureProvider> &&provider);

    void run() override;

private:
    std::shared_ptr<QSGTexture> m_texture;
    std::unique_ptr<TransientImageTextureProvider> m_provider;
};

struct TransientImageOptions {
    QUrl source;
    QSizeF size;
    qreal devicePixelRatio;
    int fillMode;

    QSize preferredSourceSize(const QSize &originalSize) const;

    bool operator==(const TransientImageOptions &other) const = default;
    bool operator!=(const TransientImageOptions &other) const = default;
};

class TransientImageReader : public QObject
{
    Q_OBJECT

public:
    explicit TransientImageReader(const TransientImageOptions &options);

    TransientImageOptions options() const;

    void start();

Q_SIGNALS:
    void finished(const QImage &image);

private:
    TransientImageOptions m_options;
};

/**
 * The TransientImage type displays an image.
 *
 * The main difference between the Image and the TransientImage types is that the latter doesn't
 * store the loaded QImage after uploading it to a scene graph texture. It reduces the memory usage,
 * but it may result in some images not being visible for a short moment after a graphics reset.
 */
class TransientImage : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)

public:
    enum Status {
        Null,
        Ready,
        Loading,
        Error,
    };
    Q_ENUM(Status)

    enum FillMode {
        Stretch,
        PreserveAspectFit,
        PreserveAspectCrop,
        Tile,
        TileVertically,
        TileHorizontally,
        Pad,
    };
    Q_ENUM(FillMode)

    explicit TransientImage(QQuickItem *parentItem = nullptr);
    ~TransientImage() override;

    QUrl source() const;
    void setSource(const QUrl &url);

    Status status() const;
    void setStatus(Status status);

    FillMode fillMode() const;
    void setFillMode(FillMode mode);

    bool isTextureProvider() const override;
    QSGTextureProvider *textureProvider() const override;
    bool eventFilter(QObject *watched, QEvent *event) override;

Q_SIGNALS:
    void sourceSizeChanged();
    void sourceChanged();
    void statusChanged();
    void fillModeChanged();

protected:
    void componentComplete() override;
    void itemChange(ItemChange change, const ItemChangeData &data) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    void releaseResources() override;

private:
    void refresh();
    void forceRefresh();
    void load();
    void clear();
    void apply(const QImage &image, const TransientImageOptions &options);

    void follow(QQuickWindow *window);

    QPointer<QQuickWindow> m_window;

    QUrl m_source;
    Status m_status = Status::Null;
    FillMode m_fillMode = FillMode::PreserveAspectCrop;
    bool m_exposed = false;

    std::optional<QImage> m_image;
    std::shared_ptr<QSGTexture> m_texture;
    mutable std::unique_ptr<TransientImageTextureProvider> m_provider;

    std::optional<TransientImageOptions> m_options;
    std::unique_ptr<TransientImageReader> m_reader;
};
