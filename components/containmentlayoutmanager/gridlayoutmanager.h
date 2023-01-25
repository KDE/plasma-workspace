/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "abstractlayoutmanager.h"
#include "appletcontainer.h"

class AppletsLayout;
class ItemContainer;

struct Geom {
    qreal x;
    qreal y;
    qreal width;
    qreal height;
    qreal rotation;
};

class GridLayoutManager : public AbstractLayoutManager
{
    Q_OBJECT

public:
    GridLayoutManager(AppletsLayout *layout);
    ~GridLayoutManager();

    void layoutGeometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    QString serializeLayout() const override;
    void parseLayout(const QString &savedLayout) override;

    bool itemIsManaged(ItemContainer *item) override;

    void resetLayout() override;
    void resetLayoutFromConfig(const QRectF &newGeom, const QRectF &oldGeom) override;

    bool restoreItem(ItemContainer *item) override;

    bool isRectAvailable(const QRectF &rect) override;

protected:
    // The rectangle as near as possible to the current item geometry which can fit it
    QRectF nextAvailableSpace(ItemContainer *item, const QSizeF &minimumSize, AppletsLayout::PreferredLayoutDirection direction) const override;

    bool assignSpaceImpl(ItemContainer *item) override;
    void releaseSpaceImpl(ItemContainer *item) override;

private:
    // Total cell rows
    inline int rows() const;

    // Total cell columns
    inline int columns() const;

    // Converts the item pixel-based geometry to a cellsize-based geometry
    inline QRect cellBasedGeometry(const QRectF &geom) const;

    // Converts the item pixel-based geometry to a cellsize-based geometry
    // This is the bounding geometry, usually larger than cellBasedGeometry
    inline QRect cellBasedBoundingGeometry(const QRectF &geom) const;

    // true if the cell is out of the bounds of the containment
    inline bool isOutOfBounds(const QPair<int, int> &cell) const;

    // True if the space for the given cell is available
    inline bool isCellAvailable(const QPair<int, int> &cell) const;

    // Returns the qrect geometry for an item
    inline QRectF itemGeometry(QQuickItem *item) const;

    // The next cell given the direction
    QPair<int, int> nextCell(const QPair<int, int> &cell, AppletsLayout::PreferredLayoutDirection direction) const;

    // The next cell that is available given the direction
    QPair<int, int> nextAvailableCell(const QPair<int, int> &cell, AppletsLayout::PreferredLayoutDirection direction) const;

    // The next cell that is has something in it given the direction
    QPair<int, int> nextTakenCell(const QPair<int, int> &cell, AppletsLayout::PreferredLayoutDirection direction) const;

    // How many cells are available in the row starting from the given cell and direction
    int freeSpaceInDirection(const QPair<int, int> &cell, AppletsLayout::PreferredLayoutDirection direction) const;

    /**
     * This reacts to changes in size hints by an item
     */
    void adjustToItemSizeHints(ItemContainer *item);

    // What is the item that occupies the point. The point is expressed in cells rather than pixels. a qpair rather a QPointF as QHash doesn't support
    // identification by QPointF
    QHash<QPair<int, int>, ItemContainer *> m_grid;
    QHash<ItemContainer *, QSet<QPair<int, int>>> m_pointsForItem;

    QHash<QString, Geom> m_parsedConfig;
};
