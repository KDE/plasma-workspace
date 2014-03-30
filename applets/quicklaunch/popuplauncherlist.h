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
#ifndef QUICKLAUNCH_POPUPLAUNCHERLIST_H
#define QUICKLAUNCH_POPUPLAUNCHERLIST_H

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
class QGraphicsLayout;
class QGraphicsLinearLayout;
class QGraphicsSceneResizeEvent;
class QGraphicsSceneDragDropEvent;
class QGraphicsSceneMouseEvent;

namespace Plasma {
    class IconWidget;
}

namespace Quicklaunch {

class DropMarker;
class Launcher;

/**
 * The PopupLauncherList is a QGraphicsWidget that displays and manages the
 * launchers on Quicklaunch's popup.
 * Since all launchers are managed by the PopupLauncherList and its layout,
 * they should not be accessed directly, but rather added or removed by
 * passing LauncherData objects.
 *
 * PopupLauncherList also takes care of drag & drop handling.
 */
class PopupLauncherList : public QGraphicsWidget
{
    Q_OBJECT

public:

    /**
     * Creates a new PopupLauncherList with the given parent item.
     *
     * @param parent the parent QGraphicsItem
     */
    explicit PopupLauncherList(QGraphicsItem *parent = 0);

    void setPreferredIconSize(int size);

    /**
     * Indicates whether this PopupLauncherList is locked and thus does not allow
     * adding, removing or reordering launchers by drag & drop.
     */
    bool locked() const;

    /**
     * Locks or unlocks this PopupLauncherList.
     *
     * @param enable whether this PopupLauncherList should be locked, thereby
     * disabling adding, removing or reordering launchers by drag & drop.
     */
    void setLocked(bool enable);

    QGraphicsLinearLayout *layout() const;

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
    QSizeF m_preferredIconSize;
    bool m_locked;

    QGraphicsLinearLayout *m_layout;

    QPointF m_mousePressedPos;
    DropMarker *m_dropMarker;
    int m_dropMarkerIndex;
    Plasma::IconWidget *m_placeHolder;
};
}

#endif /* QUICKLAUNCH_POPUPLAUNCHERLIST_H */
