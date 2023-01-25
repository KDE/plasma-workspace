/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "appletslayout.h"
#include <QObject>

class ItemContainer;

class AbstractLayoutManager : public QObject
{
    Q_OBJECT

public:
    AbstractLayoutManager(AppletsLayout *layout);
    ~AbstractLayoutManager();

    AppletsLayout *layout() const;

    void setCellSize(const QSizeF &size);
    QSizeF cellSize() const;

    /**
     * A size aligned to the gid that fully contains the given size
     */
    QSizeF cellAlignedContainingSize(const QSizeF &size) const;

    /**
     * Positions the item, does *not* assign the space as taken
     */
    void positionItem(ItemContainer *item);

    /**
     * Positions the item and assigns the space as taken by this item
     */
    void positionItemAndAssign(ItemContainer *item);

    /**
     * Set the space of item's rect as occupied by item.
     * The operation may fail if some space of the item's geometry is already occupied.
     * @returns true if the operation succeeded
     */
    bool assignSpace(ItemContainer *item);

    /**
     * If item is occupying space, set it as available
     */
    void releaseSpace(ItemContainer *item);

    // VIRTUALS
    virtual QString serializeLayout() const = 0;

    virtual void parseLayout(const QString &savedLayout) = 0;

    virtual void layoutGeometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);

    /**
     * true if the item is managed by the grid
     */
    virtual bool itemIsManaged(ItemContainer *item) = 0;

    /**
     * Forget about layout information and relayout all items based solely on their current geometry
     */
    virtual void resetLayout() = 0;

    /**
     * Forget about layout information and relayout all items based on their stored geometry first,
     * and if that fails from their current geometry.
     * If this function has been called from a resize, oldGeomety and newGeometry
     * represent the geometries that this layout had before and after the resize, for placing strategies
     * of items that weren't in the stored config.
     * If one of those rects are empty, neither should be considered in the layouting strategy.
     */
    virtual void resetLayoutFromConfig(const QRectF &newGeom, const QRectF &oldGeom) = 0;

    /**
     * Restores an item geometry from the serialized config
     * parseLayout needs to be called before this
     * @returns true if the item was stored in the config
     * and the restore has been performed.
     * Otherwise, the item is not touched and returns false
     */
    virtual bool restoreItem(ItemContainer *item) = 0;

    /**
     * @returns true if the given rectangle is all free space
     */
    virtual bool isRectAvailable(const QRectF &rect) = 0;

Q_SIGNALS:
    /**
     * Emitted when the layout has been changed and now needs saving
     */
    void layoutNeedsSaving();

protected:
    /**
     * Subclasses implement their assignSpace logic here
     */
    virtual bool assignSpaceImpl(ItemContainer *item) = 0;

    /**
     * Subclasses implement their releasespace logic here
     */
    virtual void releaseSpaceImpl(ItemContainer *item) = 0;

    /**
     * @returns a rectangle big at least as minimumSize, trying to be as near as possible to the current item's geometry, displaced in the direction we asked,
     * forwards or backwards
     * @param item the item container we want to place an item in
     * @param minimumSize the minimum size we need to make sure is available
     * @param direction the preferred item layout direction, can be Closest, LeftToRight, RightToLeft, TopToBottom, and BottomToTop
     */
    virtual QRectF nextAvailableSpace(ItemContainer *item, const QSizeF &minimumSize, AppletsLayout::PreferredLayoutDirection direction) const = 0;

private:
    QRectF candidateGeometry(ItemContainer *item) const;

    AppletsLayout *m_layout;

    // size in pixels of a crid cell
    QSizeF m_cellSize;
};
