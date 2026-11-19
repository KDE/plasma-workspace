/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "abstractlayoutmanager.h"
#include "itemcontainer.h"

#include <cmath>

AbstractLayoutManager::AbstractLayoutManager(AppletsLayout *layout)
    : QObject(layout)
    , m_layout(layout)
{
}

AbstractLayoutManager::~AbstractLayoutManager() = default;

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
    return {m_cellSize.width() * ceil(size.width() / m_cellSize.width()), m_cellSize.height() * ceil(size.height() / m_cellSize.height())};
}

void AbstractLayoutManager::setCellSize(const QSizeF &size)
{
    m_cellSize = size;
}

QRectF AbstractLayoutManager::candidateGeometry(ItemContainer *item) const
{
    const QRectF originalItemRect = QRectF(item->x(), item->y(), item->width(), item->height());

    // TODO: a default minimum size
    QSizeF minimumSize = QSize(m_layout->minimumItemWidth(), m_layout->minimumItemHeight());
    if (item->layoutAttached()) {
        minimumSize = QSizeF(qMax(minimumSize.width(), item->layoutAttached()->property("minimumWidth").toReal()),
                             qMax(minimumSize.height(), item->layoutAttached()->property("minimumHeight").toReal()));
    }

    const AppletsLayout::PreferredLayoutDirection direction = item->preferredLayoutDirection();
    AppletsLayout::PreferredLayoutDirection oppositeDirection;
    switch (direction) {
    case AppletsLayout::RightToLeft:
        oppositeDirection = AppletsLayout::LeftToRight;
        break;
    case AppletsLayout::TopToBottom:
        oppositeDirection = AppletsLayout::BottomToTop;
        break;
    case AppletsLayout::BottomToTop:
        oppositeDirection = AppletsLayout::TopToBottom;
        break;
    case AppletsLayout::LeftToRight:
    default:
        oppositeDirection = AppletsLayout::RightToLeft;
        break;
    }
    const AppletsLayout::WrapDirection wrap = item->wrapDirection();
    const AppletsLayout::WrapDirection oppositeWrap = wrap == AppletsLayout::WrapForward ? AppletsLayout::WrapBackward : AppletsLayout::WrapForward;

    // When layout mode is closest, it's a bit more expensive, we have to try to layout in all the
    // 8 possible directions then take the closest to the desired position
    if (direction == AppletsLayout::Closest) {
        QRectF ltrRect = nextAvailableSpace(item, minimumSize, AppletsLayout::LeftToRight, wrap);
        QRectF rtlRect = nextAvailableSpace(item, minimumSize, AppletsLayout::RightToLeft, wrap);
        QRectF ttbRect = nextAvailableSpace(item, minimumSize, AppletsLayout::TopToBottom, wrap);
        QRectF bttRect = nextAvailableSpace(item, minimumSize, AppletsLayout::BottomToTop, wrap);

        if (ltrRect.isEmpty()) {
            ltrRect = nextAvailableSpace(item, minimumSize, AppletsLayout::LeftToRight, oppositeWrap);
        }
        if (rtlRect.isEmpty()) {
            rtlRect = nextAvailableSpace(item, minimumSize, AppletsLayout::RightToLeft, oppositeWrap);
        }
        if (ttbRect.isEmpty()) {
            ttbRect = nextAvailableSpace(item, minimumSize, AppletsLayout::TopToBottom, oppositeWrap);
        }
        if (bttRect.isEmpty()) {
            bttRect = nextAvailableSpace(item, minimumSize, AppletsLayout::BottomToTop, oppositeWrap);
        }

        auto score = [originalItemRect](QRectF target) {
            auto dist = QPointF(originalItemRect.center() - target.center()).manhattanLength();
            auto size = QPointF(originalItemRect.width() - target.width(), originalItemRect.height() - target.height()).manhattanLength();
            return dist + size;
        };

        // Take the closest rect, unless the item prefers a particular positioning strategy
        QMap<int, QRectF> distances;
        if (!ltrRect.isEmpty()) {
            distances[score(ltrRect)] = ltrRect;
        }
        if (!rtlRect.isEmpty()) {
            distances[score(rtlRect)] = rtlRect;
        }
        if (!ttbRect.isEmpty()) {
            distances[score(ttbRect)] = ttbRect;
        }
        if (!bttRect.isEmpty()) {
            distances[score(bttRect)] = bttRect;
        }

        if (distances.isEmpty()) {
            // Failure to layout, completely full
            return originalItemRect;
        }

        return std::as_const(distances).first();
    }

    // For all the other directions, first try the desired direction,
    // if we don't find any space that fits, go backwards, opposite direction and wrap
    QRectF rect = nextAvailableSpace(item, minimumSize, direction, wrap);
    if (rect.isEmpty()) {
        rect = nextAvailableSpace(item, minimumSize, oppositeDirection, oppositeWrap);
    }
    if (rect.isEmpty()) {
        // Failure to layout, completely full
        return originalItemRect;
    }
    return rect;
}

QRectF AbstractLayoutManager::positionItem(ItemContainer *item)
{
    // Give it a sane size if uninitialized: this may change size hints
    if (item->width() <= 0 || item->height() <= 0) {
        item->setSize(QSizeF(qMax(m_layout->minimumItemWidth(), m_layout->defaultItemWidth()), //
                             qMax(m_layout->minimumItemHeight(), m_layout->defaultItemHeight())));
    }

    QRectF candidate = candidateGeometry(item);
    // Use setProperty to allow Behavior on to take effect
    item->setProperty("x", candidate.topLeft().x());
    item->setProperty("y", candidate.topLeft().y());
    item->setSize(candidate.size());
    return candidate;
}

void AbstractLayoutManager::positionItemAndAssign(ItemContainer *item)
{
    releaseSpace(item);
    assignSpace(item, positionItem(item));
}

bool AbstractLayoutManager::assignSpace(ItemContainer *item, QRectF geometry)
{
    if (assignSpaceImpl(item, geometry)) {
        Q_EMIT layoutNeedsSaving();
        return true;
    } else {
        return false;
    }
}

void AbstractLayoutManager::releaseSpace(ItemContainer *item)
{
    releaseSpaceImpl(item);
    Q_EMIT layoutNeedsSaving();
}

void AbstractLayoutManager::layoutGeometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_UNUSED(newGeometry);
    Q_UNUSED(oldGeometry);
    // NOTE: Empty base implementation, don't put anything here
}

#include "moc_abstractlayoutmanager.cpp"
