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

#include "gridlayoutmanager.h"
#include "appletslayout.h"
#include <cmath>

GridLayoutManager::GridLayoutManager(AppletsLayout *layout)
    : AbstractLayoutManager(layout)
{
}

GridLayoutManager::~GridLayoutManager()
{
}

QString GridLayoutManager::serializeLayout() const
{
    QString result;

    for (auto *item : layout()->childItems()) {
        ItemContainer *itemCont = qobject_cast<ItemContainer *>(item);
        if (itemCont && itemCont != layout()->placeHolder()) {
            result += itemCont->key() + QLatin1Char(':') + QString::number(itemCont->x()) + QLatin1Char(',') + QString::number(itemCont->y()) + QLatin1Char(',')
                + QString::number(itemCont->width()) + QLatin1Char(',') + QString::number(itemCont->height()) + QLatin1Char(',')
                + QString::number(itemCont->rotation()) + QLatin1Char(';');
        }
    }

    return result;
}

void GridLayoutManager::parseLayout(const QString &savedLayout)
{
    m_parsedConfig.clear();
    const QStringList itemsConfigs = savedLayout.split(QLatin1Char(';'));

    for (const auto &itemString : itemsConfigs) {
        QStringList itemConfig = itemString.split(QLatin1Char(':'));
        if (itemConfig.count() != 2) {
            continue;
        }

        QString id = itemConfig[0];
        QStringList itemGeom = itemConfig[1].split(QLatin1Char(','));
        if (itemGeom.count() != 5) {
            continue;
        }

        m_parsedConfig[id] = {itemGeom[0].toDouble(), itemGeom[1].toDouble(), itemGeom[2].toDouble(), itemGeom[3].toDouble(), itemGeom[4].toDouble()};
    }
}

bool GridLayoutManager::itemIsManaged(ItemContainer *item)
{
    return m_pointsForItem.contains(item);
}

inline void maintainItemEdgeAlignment(ItemContainer *item, const QRectF &newRect, const QRectF &oldRect)
{
    const qreal leftDist = item->x() - oldRect.x();
    const qreal hCenterDist = item->x() + item->width() / 2 - oldRect.center().x();
    const qreal rightDist = oldRect.right() - item->x() - item->width();

    qreal hMin = qMin(qMin(qAbs(leftDist), qAbs(hCenterDist)), qAbs(rightDist));
    if (qFuzzyCompare(hMin, qAbs(leftDist))) {
        // Right alignment, do nothing
    } else if (qFuzzyCompare(hMin, qAbs(hCenterDist))) {
        item->setX(newRect.center().x() - item->width() / 2 + hCenterDist);
    } else if (qFuzzyCompare(hMin, qAbs(rightDist))) {
        item->setX(newRect.right() - item->width() - rightDist);
    }

    const qreal topDist = item->y() - oldRect.y();
    const qreal vCenterDist = item->y() + item->height() / 2 - oldRect.center().y();
    const qreal bottomDist = oldRect.bottom() - item->y() - item->height();

    qreal vMin = qMin(qMin(qAbs(topDist), qAbs(vCenterDist)), qAbs(bottomDist));

    if (qFuzzyCompare(vMin, qAbs(topDist))) {
        // Top alignment, do nothing
    } else if (qFuzzyCompare(vMin, qAbs(vCenterDist))) {
        item->setY(newRect.center().y() - item->height() / 2 + vCenterDist);
    } else if (qFuzzyCompare(vMin, qAbs(bottomDist))) {
        item->setY(newRect.bottom() - item->height() - bottomDist);
    }
}

void GridLayoutManager::layoutGeometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    m_grid.clear();
    m_pointsForItem.clear();
    for (auto *item : layout()->childItems()) {
        // Stash the old config
        // m_parsedConfig[item->key()] = {item->x(), item->y(), item->width(), item->height(), item->rotation()};
        // Move the item to maintain the distance with the anchors point
        auto *itemCont = qobject_cast<ItemContainer *>(item);
        if (itemCont && itemCont != layout()->placeHolder()) {
            maintainItemEdgeAlignment(itemCont, newGeometry, oldGeometry);
            // NOTE: do not use positionItemAndAssign here, because we do not want to emit layoutNeedsSaving, to not save after resize
            positionItem(itemCont);
            assignSpaceImpl(itemCont);
        }
    }
}

void GridLayoutManager::resetLayout()
{
    m_grid.clear();
    m_pointsForItem.clear();
    for (auto *item : layout()->childItems()) {
        ItemContainer *itemCont = qobject_cast<ItemContainer *>(item);
        if (itemCont && itemCont != layout()->placeHolder()) {
            // NOTE: do not use positionItemAndAssign here, because we do not want to emit layoutNeedsSaving, to not save after resize
            positionItem(itemCont);
            assignSpaceImpl(itemCont);
        }
    }
}

void GridLayoutManager::resetLayoutFromConfig()
{
    m_grid.clear();
    m_pointsForItem.clear();
    QList<ItemContainer *> missingItems;

    for (auto *item : layout()->childItems()) {
        ItemContainer *itemCont = qobject_cast<ItemContainer *>(item);
        if (itemCont && itemCont != layout()->placeHolder()) {
            if (!restoreItem(itemCont)) {
                missingItems << itemCont;
            }
        }
    }

    for (auto *item : qAsConst(missingItems)) {
        // NOTE: do not use positionItemAndAssign here, because we do not want to emit layoutNeedsSaving, to not save after resize
        positionItem(item);
        assignSpaceImpl(item);
    }
}

bool GridLayoutManager::restoreItem(ItemContainer *item)
{
    auto it = m_parsedConfig.find(item->key());

    if (it != m_parsedConfig.end()) {
        // Actual restore
        item->setPosition(QPointF(it.value().x, it.value().y));
        item->setSize(QSizeF(it.value().width, it.value().height));
        item->setRotation(it.value().rotation);

        // NOTE: do not use positionItemAndAssign here, because we do not want to emit layoutNeedsSaving, to not save after resize
        // If size is empty the layout is not in a valid state and probably startup is not completed yet
        if (!layout()->size().isEmpty()) {
            releaseSpaceImpl(item);
            positionItem(item);
            assignSpaceImpl(item);
        }

        return true;
    }

    return false;
}

bool GridLayoutManager::isRectAvailable(const QRectF &rect)
{
    // TODO: define directions in which it can grow
    if (rect.x() < 0 || rect.y() < 0 || rect.x() + rect.width() > layout()->width() || rect.y() + rect.height() > layout()->height()) {
        return false;
    }

    const QRect cellItemGeom = cellBasedGeometry(rect);

    for (int row = cellItemGeom.top(); row <= cellItemGeom.bottom(); ++row) {
        for (int column = cellItemGeom.left(); column <= cellItemGeom.right(); ++column) {
            if (!isCellAvailable(QPair<int, int>(row, column))) {
                return false;
            }
        }
    }
    return true;
}

bool GridLayoutManager::assignSpaceImpl(ItemContainer *item)
{
    // Don't emit extra layoutneedssaving signals
    releaseSpaceImpl(item);
    if (!isRectAvailable(itemGeometry(item))) {
        qWarning() << "Trying to take space not available" << item;
        return false;
    }

    const QRect cellItemGeom = cellBasedGeometry(itemGeometry(item));

    for (int row = cellItemGeom.top(); row <= cellItemGeom.bottom(); ++row) {
        for (int column = cellItemGeom.left(); column <= cellItemGeom.right(); ++column) {
            QPair<int, int> cell(row, column);
            m_grid.insert(cell, item);
            m_pointsForItem[item].insert(cell);
        }
    }

    // Reorder items tab order
    for (auto *i2 : layout()->childItems()) {
        ItemContainer *item2 = qobject_cast<ItemContainer *>(i2);
        if (item2 && item2->parentItem() == item->parentItem() && item != item2 && item2 != layout()->placeHolder() && item->y() < item2->y() + item2->height()
            && item->x() <= item2->x()) {
            item->stackBefore(item2);
            break;
        }
    }

    if (item->layoutAttached()) {
        connect(item, &ItemContainer::sizeHintsChanged, this, [this, item]() {
            adjustToItemSizeHints(item);
        });
    }

    return true;
}

void GridLayoutManager::releaseSpaceImpl(ItemContainer *item)
{
    auto it = m_pointsForItem.find(item);

    if (it == m_pointsForItem.end()) {
        return;
    }

    for (const auto &point : it.value()) {
        m_grid.remove(point);
    }

    m_pointsForItem.erase(it);

    disconnect(item, &ItemContainer::sizeHintsChanged, this, nullptr);
}

int GridLayoutManager::rows() const
{
    return layout()->height() / cellSize().height();
}

int GridLayoutManager::columns() const
{
    return layout()->width() / cellSize().width();
}

void GridLayoutManager::adjustToItemSizeHints(ItemContainer *item)
{
    if (!item->layoutAttached() || item->editMode()) {
        return;
    }

    bool changed = false;

    // Minimum
    const qreal newMinimumHeight = item->layoutAttached()->property("minimumHeight").toReal();
    const qreal newMinimumWidth = item->layoutAttached()->property("minimumWidth").toReal();

    if (newMinimumHeight > item->height()) {
        item->setHeight(newMinimumHeight);
        changed = true;
    }
    if (newMinimumWidth > item->width()) {
        item->setWidth(newMinimumWidth);
        changed = true;
    }

    // Preferred
    const qreal newPreferredHeight = item->layoutAttached()->property("preferredHeight").toReal();
    const qreal newPreferredWidth = item->layoutAttached()->property("preferredWidth").toReal();

    if (newPreferredHeight > item->height()) {
        item->setHeight(layout()->cellHeight() * ceil(newPreferredHeight / layout()->cellHeight()));
        changed = true;
    }
    if (newPreferredWidth > item->width()) {
        item->setWidth(layout()->cellWidth() * ceil(newPreferredWidth / layout()->cellWidth()));
        changed = true;
    }

    /*// Maximum : IGNORE?
    const qreal newMaximumHeight = item->layoutAttached()->property("preferredHeight").toReal();
    const qreal newMaximumWidth = item->layoutAttached()->property("preferredWidth").toReal();

    if (newMaximumHeight > 0 && newMaximumHeight < height()) {
        item->setHeight(newMaximumHeight);
        changed = true;
    }
    if (newMaximumHeight > 0 && newMaximumWidth < width()) {
        item->setWidth(newMaximumWidth);
        changed = true;
    }*/

    // Relayout if anything changed
    if (changed && itemIsManaged(item)) {
        releaseSpace(item);
        positionItem(item);
        assignSpace(item);
    }
}

QRect GridLayoutManager::cellBasedGeometry(const QRectF &geom) const
{
    return QRect(round(qBound(0.0, geom.x(), layout()->width() - geom.width()) / cellSize().width()),
                 round(qBound(0.0, geom.y(), layout()->height() - geom.height()) / cellSize().height()),
                 round((qreal)geom.width() / cellSize().width()),
                 round((qreal)geom.height() / cellSize().height()));
}

QRect GridLayoutManager::cellBasedBoundingGeometry(const QRectF &geom) const
{
    return QRect(floor(qBound(0.0, geom.x(), layout()->width() - geom.width()) / cellSize().width()),
                 floor(qBound(0.0, geom.y(), layout()->height() - geom.height()) / cellSize().height()),
                 ceil((qreal)geom.width() / cellSize().width()),
                 ceil((qreal)geom.height() / cellSize().height()));
}

bool GridLayoutManager::isOutOfBounds(const QPair<int, int> &cell) const
{
    return cell.first < 0 || cell.second < 0 || cell.first >= rows() || cell.second >= columns();
}

bool GridLayoutManager::isCellAvailable(const QPair<int, int> &cell) const
{
    return !isOutOfBounds(cell) && !m_grid.contains(cell);
}

QRectF GridLayoutManager::itemGeometry(QQuickItem *item) const
{
    return QRectF(item->x(), item->y(), item->width(), item->height());
}

QPair<int, int> GridLayoutManager::nextCell(const QPair<int, int> &cell, AppletsLayout::PreferredLayoutDirection direction) const
{
    QPair<int, int> nCell = cell;

    switch (direction) {
    case AppletsLayout::AppletsLayout::BottomToTop:
        --nCell.first;
        break;
    case AppletsLayout::AppletsLayout::TopToBottom:
        ++nCell.first;
        break;
    case AppletsLayout::AppletsLayout::RightToLeft:
        --nCell.second;
        break;
    case AppletsLayout::AppletsLayout::LeftToRight:
    default:
        ++nCell.second;
        break;
    }

    return nCell;
}

QPair<int, int> GridLayoutManager::nextAvailableCell(const QPair<int, int> &cell, AppletsLayout::PreferredLayoutDirection direction) const
{
    QPair<int, int> nCell = cell;
    while (!isOutOfBounds(nCell)) {
        nCell = nextCell(nCell, direction);

        if (isOutOfBounds(nCell)) {
            switch (direction) {
            case AppletsLayout::AppletsLayout::BottomToTop:
                nCell.first = rows() - 1;
                --nCell.second;
                break;
            case AppletsLayout::AppletsLayout::TopToBottom:
                nCell.first = 0;
                ++nCell.second;
                break;
            case AppletsLayout::AppletsLayout::RightToLeft:
                --nCell.first;
                nCell.second = columns() - 1;
                break;
            case AppletsLayout::AppletsLayout::LeftToRight:
            default:
                ++nCell.first;
                nCell.second = 0;
                break;
            }
        }

        if (isCellAvailable(nCell)) {
            return nCell;
        }
    }

    return QPair<int, int>(-1, -1);
}

QPair<int, int> GridLayoutManager::nextTakenCell(const QPair<int, int> &cell, AppletsLayout::PreferredLayoutDirection direction) const
{
    QPair<int, int> nCell = cell;
    while (!isOutOfBounds(nCell)) {
        nCell = nextCell(nCell, direction);

        if (isOutOfBounds(nCell)) {
            switch (direction) {
            case AppletsLayout::AppletsLayout::BottomToTop:
                nCell.first = rows() - 1;
                --nCell.second;
                break;
            case AppletsLayout::AppletsLayout::TopToBottom:
                nCell.first = 0;
                ++nCell.second;
                break;
            case AppletsLayout::AppletsLayout::RightToLeft:
                --nCell.first;
                nCell.second = columns() - 1;
                break;
            case AppletsLayout::AppletsLayout::LeftToRight:
            default:
                ++nCell.first;
                nCell.second = 0;
                break;
            }
        }

        if (!isCellAvailable(nCell)) {
            return nCell;
        }
    }

    return QPair<int, int>(-1, -1);
}

int GridLayoutManager::freeSpaceInDirection(const QPair<int, int> &cell, AppletsLayout::PreferredLayoutDirection direction) const
{
    QPair<int, int> nCell = cell;

    int avail = 0;

    while (isCellAvailable(nCell)) {
        ++avail;
        nCell = nextCell(nCell, direction);
    }

    return avail;
}

QRectF GridLayoutManager::nextAvailableSpace(ItemContainer *item, const QSizeF &minimumSize, AppletsLayout::PreferredLayoutDirection direction) const
{
    // The mionimum size in grid units
    const QSize minimumGridSize(ceil((qreal)minimumSize.width() / cellSize().width()), ceil((qreal)minimumSize.height() / cellSize().height()));

    QRect itemCellGeom = cellBasedGeometry(itemGeometry(item));
    itemCellGeom.setWidth(qMax(itemCellGeom.width(), minimumGridSize.width()));
    itemCellGeom.setHeight(qMax(itemCellGeom.height(), minimumGridSize.height()));

    QSize partialSize;

    QPair<int, int> cell(itemCellGeom.y(), itemCellGeom.x());
    if (direction == AppletsLayout::AppletsLayout::RightToLeft) {
        cell.second += itemCellGeom.width();
    } else if (direction == AppletsLayout::AppletsLayout::BottomToTop) {
        cell.first += itemCellGeom.height();
    }

    if (!isCellAvailable(cell)) {
        cell = nextAvailableCell(cell, direction);
    }

    while (!isOutOfBounds(cell)) {
        if (direction == AppletsLayout::LeftToRight || direction == AppletsLayout::RightToLeft) {
            partialSize = QSize(INT_MAX, 0);

            int currentRow = cell.first;
            for (; currentRow < cell.first + itemCellGeom.height(); ++currentRow) {
                const int freeRow = freeSpaceInDirection(QPair<int, int>(currentRow, cell.second), direction);

                partialSize.setWidth(qMin(partialSize.width(), freeRow));

                if (freeRow > 0) {
                    partialSize.setHeight(partialSize.height() + 1);
                } else if (partialSize.height() < minimumGridSize.height()) {
                    break;
                }

                if (partialSize.width() >= itemCellGeom.width() && partialSize.height() >= itemCellGeom.height()) {
                    break;
                } else if (partialSize.width() < minimumGridSize.width()) {
                    break;
                }
            }

            if (partialSize.width() >= minimumGridSize.width() && partialSize.height() >= minimumGridSize.height()) {
                const int width = qMin(itemCellGeom.width(), partialSize.width()) * cellSize().width();
                const int height = qMin(itemCellGeom.height(), partialSize.height()) * cellSize().height();

                if (direction == AppletsLayout::RightToLeft) {
                    return QRectF((cell.second + 1) * cellSize().width() - width, cell.first * cellSize().height(), width, height);
                    // AppletsLayout::LeftToRight
                } else {
                    return QRectF(cell.second * cellSize().width(), cell.first * cellSize().height(), width, height);
                }
            } else {
                cell = nextAvailableCell(nextTakenCell(cell, direction), direction);
            }

        } else if (direction == AppletsLayout::TopToBottom || direction == AppletsLayout::BottomToTop) {
            partialSize = QSize(0, INT_MAX);

            int currentColumn = cell.second;
            for (; currentColumn < cell.second + itemCellGeom.width(); ++currentColumn) {
                const int freeColumn = freeSpaceInDirection(QPair<int, int>(cell.first, currentColumn), direction);

                partialSize.setHeight(qMin(partialSize.height(), freeColumn));

                if (freeColumn > 0) {
                    partialSize.setWidth(partialSize.width() + 1);
                } else if (partialSize.width() < minimumGridSize.width()) {
                    break;
                }

                if (partialSize.width() >= itemCellGeom.width() && partialSize.height() >= itemCellGeom.height()) {
                    break;
                } else if (partialSize.height() < minimumGridSize.height()) {
                    break;
                }
            }

            if (partialSize.width() >= minimumGridSize.width() && partialSize.height() >= minimumGridSize.height()) {
                const int width = qMin(itemCellGeom.width(), partialSize.width()) * cellSize().width();
                const int height = qMin(itemCellGeom.height(), partialSize.height()) * cellSize().height();

                if (direction == AppletsLayout::BottomToTop) {
                    return QRectF(cell.second * cellSize().width(), (cell.first + 1) * cellSize().height() - height, width, height);
                    // AppletsLayout::TopToBottom:
                } else {
                    return QRectF(cell.second * cellSize().width(), cell.first * cellSize().height(), width, height);
                }
            } else {
                cell = nextAvailableCell(nextTakenCell(cell, direction), direction);
            }
        }
    }

    // We didn't manage to find layout space, return invalid geometry
    return QRectF();
}

#include "moc_gridlayoutmanager.cpp"
