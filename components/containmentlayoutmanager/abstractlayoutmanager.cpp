/*
 *   Copyright 2019 by Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "abstractlayoutmanager.h"
#include "appletslayout.h"
#include "itemcontainer.h"

#include <cmath>

AbstractLayoutManager::AbstractLayoutManager(AppletsLayout *layout)
    : QObject(layout),
      m_layout(layout)
{
}

AbstractLayoutManager::~AbstractLayoutManager()
{
}

AppletsLayout *AbstractLayoutManager::layout() const
{
    return m_layout;
}

QSizeF AbstractLayoutManager::cellSize() const
{
    return m_cellSize;
}

QSizeF AbstractLayoutManager::cellAlignedContainingSize(const QSizeF &size) const
{
    return QSizeF(m_cellSize.width() * ceil(size.width() / m_cellSize.width()),
                  m_cellSize.height() * ceil(size.height() / m_cellSize.height()));
}

void AbstractLayoutManager::setCellSize(const QSizeF &size)
{
    m_cellSize = size;
}

QRectF AbstractLayoutManager::candidateGeometry(ItemContainer *item) const
{
    const QRectF originalItemRect = QRectF(item->x(), item->y(), item->width(), item->height());

    //TODO: a default minimum size
    QSizeF minimumSize = QSize(m_layout->minimumItemWidth(), m_layout->minimumItemHeight());
    if (item->layoutAttached()) {
        minimumSize = QSizeF(qMax(minimumSize.width(), item->layoutAttached()->property("minimumWidth").toReal()),
                            qMax(minimumSize.height(), item->layoutAttached()->property("minimumHeight").toReal()));
    }

    const QRectF ltrRect = nextAvailableSpace(item, minimumSize, AppletsLayout::LeftToRight);
    const QRectF rtlRect = nextAvailableSpace(item, minimumSize, AppletsLayout::RightToLeft);
    const QRectF ttbRect = nextAvailableSpace(item, minimumSize, AppletsLayout::TopToBottom);
    const QRectF bttRect = nextAvailableSpace(item, minimumSize, AppletsLayout::BottomToTop);

    // Take the closest rect, unless the item prefers a particular positioning strategy
    QMap <int, QRectF> distances;
    if (!ltrRect.isEmpty()) {
        const int dist = item->preferredLayoutDirection() == AppletsLayout::LeftToRight ? 0 : QPointF(originalItemRect.center() - ltrRect.center()).manhattanLength();
        distances[dist] = ltrRect;
    }
    if (!rtlRect.isEmpty()) {
        const int dist = item->preferredLayoutDirection() == AppletsLayout::RightToLeft ? 0 : QPointF(originalItemRect.center() - rtlRect.center()).manhattanLength();
        distances[dist] = rtlRect;
    }
    if (!ttbRect.isEmpty()) {
        const int dist = item->preferredLayoutDirection() == AppletsLayout::TopToBottom ? 0 : QPointF(originalItemRect.center() - ttbRect.center()).manhattanLength();
        distances[dist] = ttbRect;
    }
    if (!bttRect.isEmpty()) {
        const int dist = item->preferredLayoutDirection() == AppletsLayout::BottomToTop ? 0 : QPointF(originalItemRect.center() - bttRect.center()).manhattanLength();
        distances[dist] = bttRect;
    }

    if (distances.isEmpty()) {
        // Failure to layout, completely full
        return originalItemRect;
    } else {
        return distances.first();
    }
}

void AbstractLayoutManager::positionItem(ItemContainer *item)
{
    // Give it a sane size if uninitialized: this may change size hints
    if (item->width() <= 0 || item->height() <= 0) {
        item->setSize(QSizeF(qMax(m_layout->minimumItemWidth(), m_layout->defaultItemWidth()),
                      qMax(m_layout->minimumItemHeight(), m_layout->defaultItemHeight())));
    }

    QRectF candidate = candidateGeometry(item);
    item->setPosition(candidate.topLeft());
    item->setSize(candidate.size());
}

void AbstractLayoutManager::positionItemAndAssign(ItemContainer *item)
{
    releaseSpace(item);
    positionItem(item);
    assignSpace(item);
}

bool AbstractLayoutManager::assignSpace(ItemContainer *item)
{
    if (assignSpaceImpl(item)) {
        emit layoutNeedsSaving();
        return true;
    } else {
        return false;
    }
}

void AbstractLayoutManager::releaseSpace(ItemContainer *item)
{
    releaseSpaceImpl(item);
    emit layoutNeedsSaving();
}

void AbstractLayoutManager::layoutGeometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) {
    Q_UNUSED(newGeometry);
    Q_UNUSED(oldGeometry);
    // NOTE: Empty base implementation, don't put anything here
}

#include "moc_abstractlayoutmanager.cpp"
