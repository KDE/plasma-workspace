/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "resizehandle.h"

#include <QCursor>
#include <cmath>

ResizeHandle::ResizeHandle(QQuickItem *parent)
    : QQuickItem(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton);

    QQuickItem *candidate = parent;
    while (candidate) {
        ConfigOverlay *overlay = qobject_cast<ConfigOverlay *>(candidate);
        if (overlay) {
            setConfigOverlay(overlay);
            break;
        }

        candidate = candidate->parentItem();
    }

    connect(this, &QQuickItem::parentChanged, this, [this]() {
        QQuickItem *candidate = parentItem();
        while (candidate) {
            ConfigOverlay *overlay = qobject_cast<ConfigOverlay *>(candidate);
            if (overlay) {
                setConfigOverlay(overlay);
                break;
            }

            candidate = candidate->parentItem();
        }
    });

    auto syncCursor = [this]() {
        switch (m_resizeCorner) {
        case Left:
        case Right:
            setCursor(QCursor(Qt::SizeHorCursor));
            break;
        case Top:
        case Bottom:
            setCursor(QCursor(Qt::SizeVerCursor));
            break;
        case TopLeft:
        case BottomRight:
            setCursor(QCursor(Qt::SizeFDiagCursor));
            break;
        case TopRight:
        case BottomLeft:
        default:
            setCursor(Qt::SizeBDiagCursor);
        }
    };

    syncCursor();
    connect(this, &ResizeHandle::resizeCornerChanged, this, syncCursor);
}

ResizeHandle::~ResizeHandle()
{
}

bool ResizeHandle::resizeBlocked() const
{
    return m_resizeWidthBlocked || m_resizeHeightBlocked;
}

void ResizeHandle::setPressed(bool pressed)
{
    if (pressed == m_pressed) {
        return;
    }

    m_pressed = pressed;
    Q_EMIT pressedChanged();
}

bool ResizeHandle::isPressed() const
{
    return m_pressed;
}

bool ResizeHandle::resizeLeft() const
{
    return m_resizeCorner == Left || m_resizeCorner == TopLeft || m_resizeCorner == BottomLeft;
}

bool ResizeHandle::resizeTop() const
{
    return m_resizeCorner == Top || m_resizeCorner == TopLeft || m_resizeCorner == TopRight;
}

bool ResizeHandle::resizeRight() const
{
    return m_resizeCorner == Right || m_resizeCorner == TopRight || m_resizeCorner == BottomRight;
}

bool ResizeHandle::resizeBottom() const
{
    return m_resizeCorner == Bottom || m_resizeCorner == BottomLeft || m_resizeCorner == BottomRight;
}

void ResizeHandle::setResizeBlocked(bool width, bool height)
{
    if (m_resizeWidthBlocked == width && m_resizeHeightBlocked == height) {
        return;
    }

    m_resizeWidthBlocked = width;
    m_resizeHeightBlocked = height;

    Q_EMIT resizeBlockedChanged();
}

void ResizeHandle::mousePressEvent(QMouseEvent *event)
{
    ItemContainer *itemContainer = m_configOverlay->itemContainer();
    if (!itemContainer) {
        return;
    }
    m_mouseDownPosition = event->windowPos();
    m_mouseDownGeometry = QRectF(itemContainer->x(), itemContainer->y(), itemContainer->width(), itemContainer->height());
    setResizeBlocked(false, false);
    setPressed(true);
    event->accept();
}

void ResizeHandle::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_configOverlay || !m_configOverlay->itemContainer()) {
        return;
    }

    ItemContainer *itemContainer = m_configOverlay->itemContainer();
    AppletsLayout *layout = itemContainer->layout();

    if (!layout) {
        return;
    }

    layout->releaseSpace(itemContainer);
    const QPointF difference = m_mouseDownPosition - event->windowPos();

    QSizeF minimumSize = QSize(layout->minimumItemWidth(), layout->minimumItemHeight());
    if (itemContainer->layoutAttached()) {
        minimumSize.setWidth(qMax(minimumSize.width(), itemContainer->layoutAttached()->property("minimumWidth").toReal()));
        minimumSize.setHeight(qMax(minimumSize.height(), itemContainer->layoutAttached()->property("minimumHeight").toReal()));
    }

    // Now make minimumSize an integer number of cells
    minimumSize.setWidth(ceil(minimumSize.width() / layout->cellWidth()) * layout->cellWidth());
    minimumSize.setHeight(ceil(minimumSize.height() / layout->cellWidth()) * layout->cellHeight());

    // Horizontal resize
    if (resizeLeft()) {
        const qreal width = qMax(minimumSize.width(), m_mouseDownGeometry.width() + difference.x());
        const qreal x = m_mouseDownGeometry.x() + (m_mouseDownGeometry.width() - width);

        // -1 to have a bit of margins around
        if (layout->isRectAvailable(x - 1, m_mouseDownGeometry.y(), width, m_mouseDownGeometry.height())) {
            itemContainer->setX(x);
            itemContainer->setWidth(width);
            setResizeBlocked(m_mouseDownGeometry.width() + difference.x() < minimumSize.width(), m_resizeHeightBlocked);
        } else {
            setResizeBlocked(true, m_resizeHeightBlocked);
        }
    } else if (resizeRight()) {
        const qreal width = qMax(minimumSize.width(), m_mouseDownGeometry.width() - difference.x());

        if (layout->isRectAvailable(m_mouseDownGeometry.x(), m_mouseDownGeometry.y(), width, m_mouseDownGeometry.height())) {
            itemContainer->setWidth(width);
            setResizeBlocked(m_mouseDownGeometry.width() - difference.x() < minimumSize.width(), m_resizeHeightBlocked);
        } else {
            setResizeBlocked(true, m_resizeHeightBlocked);
        }
    }

    // Vertical Resize
    if (resizeTop()) {
        const qreal height = qMax(minimumSize.height(), m_mouseDownGeometry.height() + difference.y());
        const qreal y = m_mouseDownGeometry.y() + (m_mouseDownGeometry.height() - height);

        // -1 to have a bit of margins around
        if (layout->isRectAvailable(m_mouseDownGeometry.x(), y - 1, m_mouseDownGeometry.width(), m_mouseDownGeometry.height())) {
            itemContainer->setY(y);
            itemContainer->setHeight(height);
            setResizeBlocked(m_resizeWidthBlocked, m_mouseDownGeometry.height() + difference.y() < minimumSize.height());
        } else {
            setResizeBlocked(m_resizeWidthBlocked, true);
        }
    } else if (resizeBottom()) {
        const qreal height = qMax(minimumSize.height(), m_mouseDownGeometry.height() - difference.y());

        if (layout->isRectAvailable(m_mouseDownGeometry.x(), m_mouseDownGeometry.y(), m_mouseDownGeometry.width(), height)) {
            itemContainer->setHeight(qMax(height, minimumSize.height()));
            setResizeBlocked(m_resizeWidthBlocked, m_mouseDownGeometry.height() - difference.y() < minimumSize.height());
        } else {
            setResizeBlocked(m_resizeWidthBlocked, true);
        }
    }

    event->accept();
}

void ResizeHandle::mouseReleaseEvent(QMouseEvent *event)
{
    setPressed(false);
    if (!m_configOverlay || !m_configOverlay->itemContainer()) {
        return;
    }

    ItemContainer *itemContainer = m_configOverlay->itemContainer();
    AppletsLayout *layout = itemContainer->layout();

    if (!layout) {
        return;
    }

    layout->positionItem(itemContainer);

    event->accept();

    setResizeBlocked(false, false);
    Q_EMIT resizeBlockedChanged();
}

void ResizeHandle::mouseUngrabEvent()
{
    setPressed(false);
}

void ResizeHandle::setConfigOverlay(ConfigOverlay *handle)
{
    if (handle == m_configOverlay) {
        return;
    }

    m_configOverlay = handle;
}

#include "moc_resizehandle.cpp"
