/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "transientimage.h"
#include "finder/packagefinder.h"

#include <KPackage/PackageLoader>

#include <QFuture>
#include <QImageReader>
#include <QPainter>
#include <QSGImageNode>
#include <QSGTexture>
#include <QSvgRenderer>
#include <QtConcurrent>

TransientImageNode::TransientImageNode(QQuickWindow *window)
    : m_node(window->createImageNode())
{
    appendChildNode(m_node.get());
}

void TransientImageNode::setTargetRect(const QRectF &rect)
{
    m_node->setRect(rect);
}

void TransientImageNode::setSourceRect(const QRectF &rect)
{
    m_node->setSourceRect(rect);
}

void TransientImageNode::setTexture(std::shared_ptr<QSGTexture> texture)
{
    if (m_texture == texture) {
        return;
    }

    m_texture = texture;
    m_node->setTexture(m_texture.get());
}

void TransientImageNode::setFiltering(QSGTexture::Filtering filtering)
{
    m_node->setFiltering(filtering);
}

TransientImageTextureProvider::TransientImageTextureProvider()
{
}

QSGTexture *TransientImageTextureProvider::texture() const
{
    return m_texture.get();
}

void TransientImageTextureProvider::setTexture(std::shared_ptr<QSGTexture> texture)
{
    if (m_texture != texture) {
        m_texture = texture;
        Q_EMIT textureChanged();
    }
}

TransientImageCleanupJob::TransientImageCleanupJob(std::shared_ptr<QSGTexture> &&texture, std::unique_ptr<TransientImageTextureProvider> &&provider)
    : m_texture(std::move(texture))
    , m_provider(std::move(provider))
{
}

void TransientImageCleanupJob::run()
{
    m_texture.reset();
    m_provider.reset();
}

TransientImage::TransientImage(QQuickItem *parentItem)
    : QQuickItem(parentItem)
{
    setFlag(ItemHasContents);
}

TransientImage::~TransientImage()
{
    if (m_provider || m_texture) {
        window()->scheduleRenderJob(new TransientImageCleanupJob(std::move(m_texture), std::move(m_provider)), QQuickWindow::NoStage);
    }
}

QUrl TransientImage::source() const
{
    return m_source;
}

void TransientImage::setSource(const QUrl &url)
{
    if (m_source != url) {
        m_source = url;
        if (isComponentComplete()) {
            refresh();
        }
        Q_EMIT sourceChanged();
    }
}

TransientImage::Status TransientImage::status() const
{
    return m_status;
}

void TransientImage::setStatus(Status status)
{
    if (m_status != status) {
        m_status = status;
        Q_EMIT statusChanged();
    }
}

TransientImage::FillMode TransientImage::fillMode() const
{
    return m_fillMode;
}

void TransientImage::setFillMode(FillMode mode)
{
    if (m_fillMode != mode) {
        m_fillMode = mode;
        if (isComponentComplete()) {
            refresh();
        }
        Q_EMIT fillModeChanged();
    }
}

bool TransientImage::isTextureProvider() const
{
    return true;
}

QSGTextureProvider *TransientImage::textureProvider() const
{
    if (!m_provider) {
        m_provider = std::make_unique<TransientImageTextureProvider>();
        m_provider->setTexture(m_texture);
    }

    return m_provider.get();
}

bool TransientImage::eventFilter(QObject *watched, QEvent *event)
{
    if (m_window == watched) {
        if (event->type() == QEvent::Expose) {
            if (m_exposed != m_window->isExposed()) {
                m_exposed = m_window->isExposed();
                refresh();
            }
        }
    }

    return QQuickItem::eventFilter(watched, event);
}

static QSizeF scaleAndCrop(const QSizeF &bounds, const QSizeF &size)
{
    const qreal width = bounds.width() * size.height() / bounds.height();
    if (width <= size.width()) {
        return QSizeF(width, size.height());
    } else {
        return QSizeF(size.width(), bounds.height() * size.width() / bounds.width());
    }
}

static QSizeF scaleAndCover(const QSizeF &bounds, const QSizeF &size)
{
    const qreal widthScale = bounds.width() / size.width();
    const qreal heightScale = bounds.height() / size.height();

    if (widthScale > heightScale) {
        return QSizeF(bounds.width(), widthScale * size.height());
    } else {
        return QSizeF(heightScale * size.width(), bounds.height());
    }
}

static QSizeF scaleAndFit(const QSizeF &bounds, const QSizeF &size)
{
    const qreal widthScale = bounds.width() / size.width();
    const qreal heightScale = bounds.height() / size.height();

    if (widthScale <= heightScale) {
        return QSizeF(bounds.width(), widthScale * size.height());
    } else {
        return QSizeF(heightScale * size.width(), bounds.height());
    }
}

QSize TransientImageOptions::preferredSourceSize(const QSize &originalSize) const
{
    const QSize requestedSize = (size * devicePixelRatio).toSize();
    switch (TransientImage::FillMode(fillMode)) {
    case TransientImage::Tile:
    case TransientImage::TileHorizontally:
    case TransientImage::TileVertically:
    case TransientImage::Pad:
        return originalSize;

    case TransientImage::Stretch:
        return requestedSize;

    case TransientImage::FillMode::PreserveAspectCrop:
        if (originalSize.width() < requestedSize.width() || originalSize.height() < requestedSize.height()) {
            return originalSize;
        } else {
            return scaleAndCover(requestedSize, originalSize).toSize();
        }

    case TransientImage::FillMode::PreserveAspectFit:
        if (originalSize.width() < requestedSize.width() || originalSize.height() < requestedSize.height()) {
            return originalSize;
        } else {
            return scaleAndFit(requestedSize, originalSize).toSize();
        }
    }

    Q_UNREACHABLE();
}

TransientImageReader::TransientImageReader(const TransientImageOptions &options)
    : m_options(options)
{
}

TransientImageOptions TransientImageReader::options() const
{
    return m_options;
}

static QImage loadPackage(const TransientImageOptions &options)
{
    const QUrlQuery urlQuery(options.source);
    const QString dir = urlQuery.queryItemValue(QStringLiteral("dir"));
    if (dir.isEmpty()) {
        return QImage();
    }

    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));
    package.setPath(dir);

    if (!package.isValid()) {
        return QImage();
    }

    const QSize requestedSize = (options.size * options.devicePixelRatio).toSize();
    WallpaperPackage::findPreferredImageInPackage(package, requestedSize);

    QString path = package.filePath("preferred");
    if (urlQuery.queryItemValue(QStringLiteral("darkMode")).toInt() == 1) {
        const QString darkPath = package.filePath("preferredDark");
        if (!darkPath.isEmpty()) {
            path = darkPath;
        }
    }

    if (path.endsWith(QLatin1String(".svg")) || path.endsWith(QLatin1String(".svgz"))) {
        QSvgRenderer renderer(path);

        QImage image(renderer.defaultSize().scaled(requestedSize, Qt::KeepAspectRatioByExpanding), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        renderer.render(&painter, image.rect());
        painter.end();

        return image;
    } else {
        QImageReader reader(path);
        reader.setAutoTransform(true);
        reader.setScaledSize(options.preferredSourceSize(reader.size()));

        QImage image;
        if (reader.read(&image)) {
            return image;
        }
    }

    return QImage();
}

static QImage loadSimpleImage(const TransientImageOptions &options)
{
    QImageReader reader(options.source.toLocalFile());
    reader.setAutoTransform(true);
    reader.setScaledSize(options.preferredSourceSize(reader.size()));

    QImage image = reader.read();
    image.setDevicePixelRatio(options.devicePixelRatio);

    return image;
}

static QImage loadImage(const TransientImageOptions &options)
{
    if (options.source.scheme() == QLatin1String("image")) {
        if (options.source.host() == QLatin1String("package")) {
            return loadPackage(options);
        } else {
            return QImage();
        }
    }

    if (options.source.isLocalFile()) {
        return loadSimpleImage(options);
    }

    return QImage();
}

void TransientImageReader::start()
{
    QtConcurrent::run(loadImage, m_options).then(this, [this](const QImage &image) {
        Q_EMIT finished(image);
    });
}

void TransientImage::refresh()
{
    if (source().isEmpty()) {
        clear();
    } else {
        load();
    }
}

void TransientImage::forceRefresh()
{
    m_options.reset();

    refresh();
}

void TransientImage::load()
{
    if (size().isEmpty() || !m_exposed) {
        setStatus(Status::Loading);
        m_reader.reset();
        return;
    }

    const TransientImageOptions options{
        .source = source(),
        .size = size(),
        .devicePixelRatio = window()->effectiveDevicePixelRatio(),
        .fillMode = fillMode(),
    };

    if (m_reader) {
        if (m_reader->options() == options) {
            return;
        }
        m_reader.reset();
    }

    if (m_options == options) {
        setStatus(Status::Ready);
        return;
    }

    m_reader = std::make_unique<TransientImageReader>(options);
    connect(m_reader.get(), &TransientImageReader::finished, this, [this](const QImage &image) {
        apply(image, m_reader->options());
        m_reader.reset();
    });

    setStatus(Status::Loading);
    m_reader->start();
}

void TransientImage::clear()
{
    if (m_status == Status::Null) {
        return;
    }

    m_reader.reset();
    m_options.reset();

    setImplicitSize(0, 0);
    setStatus(Status::Null);

    m_image = QImage();
    update();
}

void TransientImage::apply(const QImage &image, const TransientImageOptions &options)
{
    m_image = image;
    m_options = options;

    const QSizeF preferredSize = image.deviceIndependentSize();
    setImplicitSize(preferredSize.width(), preferredSize.height());
    setStatus(m_image->isNull() ? Status::Error : Status::Ready);

    update();
}

void TransientImage::follow(QQuickWindow *window)
{
    if (m_window == window) {
        return;
    }

    if (m_window) {
        m_window->removeEventFilter(this);
        disconnect(m_window, &QQuickWindow::sceneGraphInitialized, this, &TransientImage::forceRefresh);
    }

    m_window = window;
    m_exposed = window ? window->isExposed() : false;

    if (m_window) {
        m_window->installEventFilter(this);
        connect(m_window, &QQuickWindow::sceneGraphInitialized, this, &TransientImage::forceRefresh);
    }
}

void TransientImage::componentComplete()
{
    QQuickItem::componentComplete();

    follow(window());
    refresh();
}

void TransientImage::itemChange(ItemChange change, const ItemChangeData &data)
{
    QQuickItem::itemChange(change, data);

    if (isComponentComplete()) {
        if (change == ItemDevicePixelRatioHasChanged) {
            refresh();
        } else if (change == ItemSceneChange) {
            follow(window());
            refresh();
        }
    }
}

void TransientImage::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);

    if (isComponentComplete()) {
        refresh();
    }
}

QSGNode *TransientImage::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (const auto image = std::exchange(m_image, std::nullopt)) {
        if (image->isNull()) {
            m_texture.reset();
        } else {
            m_texture.reset(window()->createTextureFromImage(*image));
        }
    }

    if (m_provider) {
        m_provider->setTexture(m_texture);
    }

    if (!m_texture) {
        delete oldNode;
        return nullptr;
    }

    const qreal devicePixelRatio = window()->effectiveDevicePixelRatio();
    const QSize textureSize = m_texture->textureSize();

    QRectF sourceRect;
    QRectF targetRect;

    switch (m_fillMode) {
    case FillMode::Pad: {
        const qreal imageWidth = textureSize.width() / devicePixelRatio;
        const qreal imageHeight = textureSize.height() / devicePixelRatio;

        const qreal targetWidth = std::min(width(), imageWidth);
        const qreal targetHeight = std::min(height(), imageHeight);
        const qreal targetX = 0.5 * (width() - targetWidth);
        const qreal targetY = 0.5 * (height() - targetHeight);

        const int sourceWidth = std::round(targetWidth * devicePixelRatio);
        const int sourceHeight = std::round(targetHeight * devicePixelRatio);
        const int sourceX = std::floor(0.5 * (textureSize.width() - sourceWidth));
        const int sourceY = std::floor(0.5 * (textureSize.height() - sourceHeight));

        sourceRect = QRectF(sourceX, sourceY, sourceWidth, sourceHeight);
        targetRect = QRectF(targetX, targetY, targetWidth, targetHeight);
        break;
    }

    case FillMode::PreserveAspectCrop: {
        const QSize sourceSize = scaleAndCrop(size() * devicePixelRatio, textureSize).toSize();
        const int sourceX = std::floor((textureSize.width() - sourceSize.width()) * 0.5);
        const int sourceY = std::floor((textureSize.height() - sourceSize.height()) * 0.5);

        sourceRect = QRectF(sourceX, sourceY, sourceSize.width(), sourceSize.height());
        targetRect = QRectF(0, 0, width(), height());
        break;
    }

    case FillMode::PreserveAspectFit: {
        const QSizeF targetSize = scaleAndFit(size(), textureSize / devicePixelRatio);
        const qreal targetX = (width() - targetSize.width()) * 0.5;
        const qreal targetY = (height() - targetSize.height()) * 0.5;

        sourceRect = QRectF(0, 0, textureSize.width(), textureSize.height());
        targetRect = QRectF(targetX, targetY, targetSize.width(), targetSize.height());
        break;
    }

    case FillMode::Tile:
    case FillMode::TileHorizontally:
    case FillMode::TileVertically:
    case FillMode::Stretch:
        sourceRect = QRectF(QPoint(0, 0), textureSize);
        targetRect = QRectF(QPoint(0, 0), size());
        break;
    }

    TransientImageNode *node = static_cast<TransientImageNode *>(oldNode);
    if (!node) {
        node = new TransientImageNode(window());
    }

    node->setSourceRect(sourceRect);
    node->setTargetRect(targetRect);
    node->setFiltering(QSGTexture::Linear);
    node->setTexture(m_texture);

    return node;
}

void TransientImage::releaseResources()
{
    if (m_provider || m_texture) {
        window()->scheduleRenderJob(new TransientImageCleanupJob(std::move(m_texture), std::move(m_provider)), QQuickWindow::NoStage);
    }
}
