/*
    SPDX-FileCopyrightText: 2003-2007 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: GPL-2.0-only
*/

#include <QMouseEvent>
#include <QPainter>
#include <QQuickRenderControl>
#include <QQuickWindow>

#include <KWindowSystem>
#include <algorithm>

#include "previewwidget.h"

#include "config-X11.h"
#include "cursortheme.h"

namespace
{
// Preview cursors
constexpr const char *const cursor_names[] = {
    "left_ptr",
    "left_ptr_watch",
    "wait",
    "pointer",
    "help",
    "ibeam",
    "size_all",
    "size_fdiag",
    "cross",
    "split_h",
    "size_ver",
    "size_hor",
    "size_bdiag",
    "split_v",
};

const qreal widgetMinWidth = 10; // The minimum width of the preview widget
const qreal widgetMinHeight = 48; // The minimum height of the preview widget
}

class PreviewCursor
{
public:
    PreviewCursor(const CursorTheme *theme, const QString &name, int size);

    int width() const
    {
        if (!m_images.empty()) {
            return m_images.front().image.width();
        }
        return 0;
    }
    int height() const
    {
        if (!m_images.empty()) {
            return m_images.front().image.height();
        }
        return 0;
    }
    int boundingSize() const
    {
        return m_boundingSize;
    }
    inline QRectF rect() const;
    void setPosition(const QPoint &p)
    {
        m_pos = p;
    }
    void setPosition(int x, int y)
    {
        m_pos = QPoint(x, y);
    }
    QPoint position() const
    {
        return m_pos;
    }
    const std::vector<CursorTheme::CursorImage> &images() const
    {
        return m_images;
    }

private:
    int m_boundingSize;
    std::vector<CursorTheme::CursorImage> m_images;
    QPoint m_pos;
};

PreviewCursor::PreviewCursor(const CursorTheme *theme, const QString &name, int size)
    : m_boundingSize(size > 0 ? size : theme->defaultCursorSize())
    , m_images(theme->loadImages(name, size))
{
}

QRectF PreviewCursor::rect() const
{
    return QRectF(m_pos, QSizeF(width(), height()));
}

// ------------------------------------------------------------------------------

PreviewWidget::PreviewWidget(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_currentIndex(-1)
    , m_currentSize(0)
{
    setAcceptHoverEvents(true);
    current = nullptr;
    connect(&m_animationTimer, &QTimer::timeout, this, [this] {
        Q_ASSERT(current);
        const auto &frame = current->images().at(nextAnimationFrame);
        setCursor(QCursor(QPixmap::fromImage(frame.image), frame.hotspot.x(), frame.hotspot.y()));
        m_animationTimer.setInterval(frame.delay);
        nextAnimationFrame = (nextAnimationFrame + 1) % current->images().size();
    });
}

PreviewWidget::~PreviewWidget()
{
    qDeleteAll(list);
    list.clear();
}

void PreviewWidget::componentComplete()
{
    QQuickPaintedItem::componentComplete();
    refresh();
}

void PreviewWidget::setThemeModel(SortProxyModel *themeModel)
{
    if (m_themeModel == themeModel) {
        return;
    }

    m_themeModel = themeModel;
    Q_EMIT themeModelChanged();
}

SortProxyModel *PreviewWidget::themeModel()
{
    return m_themeModel;
}

void PreviewWidget::setCurrentIndex(int idx)
{
    if (m_currentIndex == idx) {
        return;
    }

    m_currentIndex = idx;
    refresh();
    Q_EMIT currentIndexChanged();
}

int PreviewWidget::currentIndex() const
{
    return m_currentIndex;
}

void PreviewWidget::setCurrentSize(int size)
{
    if (m_currentSize == size) {
        return;
    }

    m_currentSize = size;
    refresh();
    Q_EMIT currentSizeChanged();
}

int PreviewWidget::currentSize() const
{
    return m_currentSize;
}

int PreviewWidget::maximumCount() const
{
    return m_maximumCount;
}

void PreviewWidget::setMaximumCount(int maximumCount)
{
    if (m_maximumCount == maximumCount) {
        return;
    }

    m_maximumCount = maximumCount;
    refresh();
    Q_EMIT maximumCountChanged(maximumCount);
}

int PreviewWidget::padding() const
{
    return m_padding;
}

void PreviewWidget::setPadding(int padding)
{
    if (m_padding == padding) {
        return;
    }

    m_padding = padding;
    refresh(); // could just relayout?
    Q_EMIT paddingChanged(padding);
}

int PreviewWidget::spacing() const
{
    return m_spacing;
}

void PreviewWidget::setSpacing(int spacing)
{
    if (m_spacing == spacing) {
        return;
    }

    m_spacing = spacing;
    refresh(); // could just relayout?
    Q_EMIT spacingChanged(spacing);
}

void PreviewWidget::refresh()
{
    if (!isComponentComplete()) {
        return;
    }

    if (!m_themeModel) {
        return;
    }

    const CursorTheme *theme = m_themeModel->theme(m_themeModel->index(m_currentIndex, 0));
    setTheme(theme, m_currentSize);
}

void PreviewWidget::updateImplicitSize()
{
    qreal totalWidth = 0;
    qreal maxHeight = 0;

    for (const auto *c : std::as_const(list)) {
        totalWidth += c->width();
        maxHeight = qMax(c->height(), (int)maxHeight);
    }

    totalWidth += (list.count() - 1) * m_spacing;
    maxHeight = qMax(maxHeight, widgetMinHeight);

    setImplicitWidth(qMax(totalWidth, widgetMinWidth));
    setImplicitHeight(qMax(height(), maxHeight));
}

void PreviewWidget::layoutItems()
{
    if (!list.isEmpty()) {
        double deviceCoordinateWidth = width();
#if HAVE_X11
        if (KWindowSystem::isPlatformX11()) {
            deviceCoordinateWidth *= window()->devicePixelRatio();
        }
#endif
        int nextX = m_padding;
        int nextY = m_padding;

        for (auto *c : std::as_const(list)) {
            c->setPosition(nextX, nextY);
            const int boundingSize = c->boundingSize();
            nextX += boundingSize + m_spacing;
            if (nextX + boundingSize > deviceCoordinateWidth - m_padding) {
                nextX = m_padding;
                nextY += boundingSize + m_spacing;
            }
        }
    }

    needLayout = false;
}

void PreviewWidget::setTheme(const CursorTheme *theme, const int size)
{
    qDeleteAll(list);
    list.clear();

    if (theme) {
        int numCursors = m_maximumCount;
        if (numCursors <= 0) {
            numCursors = sizeof(cursor_names) / sizeof(cursor_names[0]);
        }
        for (int i = 0; i < numCursors; i++) {
            list << new PreviewCursor(theme, QString::fromLatin1(cursor_names[i]), size);
        }

        needLayout = true;
        updateImplicitSize();
    }

    current = nullptr;
    m_animationTimer.stop();
    update();
}

void PreviewWidget::paint(QPainter *painter)
{
    if (needLayout)
        layoutItems();

    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

    // for cursor themes we must ignore the native scaling,
    // as they will be rendered by X11/KWin, ignoring whatever Qt
    // scaling
    double devicePixelRatio = 1;
#if HAVE_X11
    if (KWindowSystem::isPlatformX11()) {
        devicePixelRatio = window()->devicePixelRatio();
    }
    painter->scale(1 / devicePixelRatio, 1 / devicePixelRatio);
#endif
    for (const auto *c : std::as_const(list)) {
        const QImage image = !c->images().empty() ? c->images().front().image : QImage();
        if (image.isNull()) {
            continue;
        }

        painter->drawImage(c->position(), image);
    }
}

void PreviewWidget::hoverMoveEvent(QHoverEvent *e)
{
    e->ignore(); // Propagate hover event to parent

    if (needLayout)
        layoutItems();

    double devicePixelRatio = 1.0;
#if HAVE_X11
    if (KWindowSystem::isPlatformX11()) {
        devicePixelRatio = window()->devicePixelRatio();
    }
#endif
    auto it = std::ranges::find_if(list, [this, e, devicePixelRatio](const PreviewCursor *c) {
        // Increase hit area.
        const int padding = m_spacing / 2;
        const auto adjustedRect = c->rect().adjusted(-padding, -padding, +padding, +padding);
        return adjustedRect.contains(e->position() * devicePixelRatio);
    });
    const PreviewCursor *cursor = it != list.cend() ? *it : nullptr;

    if (cursor == std::exchange(current, cursor)) {
        return;
    }
    m_animationTimer.stop();

    if (current == nullptr) {
        setCursor(Qt::ArrowCursor);
        return;
    }

    const auto &images = current->images();
    if (images.empty()) {
        setCursor(Qt::ArrowCursor);
        return;
    }

    if (images.size() == 1) {
        const auto &image = images.front();
        setCursor(QCursor(QPixmap::fromImage(image.image), image.hotspot.x(), image.hotspot.y()));
        return;
    }

    nextAnimationFrame = 0;
    m_animationTimer.setInterval(0);
    m_animationTimer.start();
}

void PreviewWidget::hoverLeaveEvent(QHoverEvent *e)
{
    m_animationTimer.stop();
    unsetCursor();

    e->ignore(); // Propagate hover event to parent
}

void PreviewWidget::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_UNUSED(newGeometry)
    Q_UNUSED(oldGeometry)
    if (!list.isEmpty()) {
        needLayout = true;
    }
}
