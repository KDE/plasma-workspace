/***************************************************************************
 *   Copyright (C) 2010 - 2011 by Ingomar Wesp <ingomar@wesp.name>         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 ***************************************************************************/
#ifndef QUICKLAUNCH_LAUNCHERGRID_H
#define QUICKLAUNCH_LAUNCHERGRID_H

// Qt
#include <Qt>
#include <QtGlobal>
#include <QtCore/QList>
#include <QtCore/QPointF>
#include <QtGui/QGraphicsWidget>

// KDE
#include <KUrl>

// Plasma
#include <Plasma/Plasma>

// Own
#include "launcherdata.h"

class QAction;
class QEvent;
class QGraphicsItem;
class QGraphicsSceneResizeEvent;
class QGraphicsSceneDragDropEvent;
class QGraphicsSceneMouseEvent;

namespace Plasma {
    class IconWidget;
}

namespace Quicklaunch {

class DropMarker;
class IconGridLayout;
class Launcher;

/**
 * The LauncherGrid is a QGraphicsWidget that displays and manages a grid of
 * multiple Launchers.
 * Since all launchers are managed by the LauncherGrid and its layout, they
 * should not be accessed directly, but rather added or removed by passing
 * LauncherData objects.
 *
 * LauncherGrid also takes care of drag & drop handling.
 */
class LauncherGrid : public QGraphicsWidget
{
    Q_OBJECT

public:

    enum LayoutMode {
        PreferColumns, /**< Prefer columns over rows. */
        PreferRows     /**< Prefer rows over columns. */
    };

    /**
     * Creates a new LauncherGrid with the given parent item.
     *
     * @param parent the parent QGraphicsItem
     */
    explicit LauncherGrid(QGraphicsItem *parent = 0);

    bool launcherNamesVisible() const;
    void setLauncherNamesVisible(bool enable);
    void setPreferredIconSize(int size);

    LayoutMode layoutMode() const;
    void setLayoutMode(LayoutMode);

    int maxSectionCount() const;

   /**
    * Depending on the mode, @c setMaxSectionCount limits either the
    * number of rows or the number of columns that are displayed. In
    * @c PreferColumns mode, @a maxSectionCount limites the maximum
    * number of columns while in @c PreferRows mode, it applies to
    * the maximum number of rows.
    *
    * Setting @a maxSectionCount to @c 0 disables the limitation.
    *
    * @param maxSectionCount the maximum number of rows or columns
    *    (depending on the mode) that should be displayed.
    */
   void setMaxSectionCount(int maxSectionCount);

   bool maxSectionCountForced() const;
   void setMaxSectionCountForced(bool enable);

    /**
     * Indicates whether this LauncherGrid is locked and thus does not allow
     * adding, removing or reordering launchers by drag & drop.
     */
    bool locked() const;

    /**
     * Locks or unlocks this LauncherGrid.
     *
     * @param enable whether this LauncherGrid should be locked, thereby
     * disabling adding, removing or reordering launchers by drag & drop.
     */
    void setLocked(bool enable);

    IconGridLayout *layout() const;

    int launcherCount() const;

    void clear();
    void insert(int index, const LauncherData &launcherData);
    void insert(int index, const QList<LauncherData> &launcherDataList);
    void removeAt(int index);
    LauncherData launcherAt(int index) const;
    int launcherIndexAtPosition(const QPointF& pos) const;

    bool eventFilter(QObject *watched, QEvent *event);

Q_SIGNALS:
    /**
     * Indicates a change to one or more of the displayed launchers.
     */
    void launchersChanged();

    /**
     * Indicates that one of the launcher items was clicked.
     */
    void launcherClicked();

protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
    void dragMoveEvent(QGraphicsSceneDragDropEvent *event);
    void dragLeaveEvent(QGraphicsSceneDragDropEvent *event);
    void dropEvent(QGraphicsSceneDragDropEvent *event);

private Q_SLOTS:
    void onPlaceHolderActivated();

private:
    void initPlaceHolder();
    void deletePlaceHolder();
    int determineDropMarkerIndex(const QPointF &localPos) const;

    QList<Launcher*> m_launchers;
    bool m_launcherNamesVisible;
    QSizeF m_preferredIconSize;
    bool m_locked;

    IconGridLayout *m_layout;

    QPointF m_mousePressedPos;
    DropMarker *m_dropMarker;
    int m_dropMarkerIndex;
    Plasma::IconWidget *m_placeHolder;
};
}

#endif /* QUICKLAUNCH_LAUNCHERGRID_H */
