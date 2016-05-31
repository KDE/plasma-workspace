/********************************************************************
Copyright 2016  Eike Hein <hein.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) version 3, or any
later version accepted by the membership of KDE e.V. (or its
successor approved by the membership of KDE e.V.), which shall
act as a proxy defined in Section 6 of version 3 of the license.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#ifndef TASKFILTERPROXYMODEL_H
#define TASKFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

#include "abstracttasksmodeliface.h"

#include "taskmanager_export.h"

namespace TaskManager
{

/**
 * @short A proxy tasks model filtering its source model by various properties.
 *
 * This proxy model class filters its source tasks model by properties such as
 * virtual desktop or minimixed state. The values to filter for or by are set
 * as properties on the proxy model instance.
 *
 * @author Eike Hein <hein@kde.org>
 **/

class TASKMANAGER_EXPORT TaskFilterProxyModel : public QSortFilterProxyModel, public AbstractTasksModelIface
{
    Q_OBJECT

    Q_PROPERTY(int virtualDesktop READ virtualDesktop WRITE setVirtualDesktop NOTIFY virtualDesktopChanged)
    Q_PROPERTY(int screen READ screen WRITE setScreen NOTIFY screenChanged)
    Q_PROPERTY(QString activity READ activity WRITE setActivity NOTIFY activityChanged)

    Q_PROPERTY(bool filterByVirtualDesktop READ filterByVirtualDesktop WRITE setFilterByVirtualDesktop NOTIFY filterByVirtualDesktopChanged)
    Q_PROPERTY(bool filterByScreen READ filterByScreen WRITE setFilterByScreen NOTIFY filterByScreenChanged)
    Q_PROPERTY(bool filterByActivity READ filterByActivity WRITE setFilterByActivity NOTIFY filterByActivityChanged)
    Q_PROPERTY(bool filterNotMinimized READ filterNotMinimized WRITE setFilterNotMinimized NOTIFY filterNotMinimizedChanged)

public:
    explicit TaskFilterProxyModel(QObject *parent = 0);
    virtual ~TaskFilterProxyModel();;

    void setSourceModel(QAbstractItemModel *sourceModel) override;

    /**
     * The number of the virtual desktop used in filtering by virtual
     * desktop. Usually set to the number of the current virtual desktop.
     * Defaults to @c 0 (virtual desktop numbers start at 1).
     *
     * @see setVirtualDesktop
     * @returns the number of the virtual desktop used in filtering.
     **/
    uint virtualDesktop() const;

    /**
     * Set the number of the virtual desktop to use in filtering by virtual
     * desktop.
     *
     * If set to 0 (virtual desktop numbers start at 1), filtering by virtual
     * desktop is disabled.
     *
     * @see virtualDesktop
     * @param virtualDesktop A virtual desktop number.
     **/
    void setVirtualDesktop(uint virtualDesktop);

    /**
     * The number of the screen used in filtering by screen. Usually
     * set to the number of the current screen. Defaults to @c -1.
     *
     * @see setScreen
     * @returns the number of the screen used in filtering.
     **/
    int screen() const;

    /**
     * Set the number of the screen to use in filtering by screen.
     *
     * If set to @c -1, filtering by screen is disabled.
     *
     * @see screen
     * @param screen A screen number.
     **/
    void setScreen(int screen);

    /**
     * The id of the activity used in filtering by activity. Usually
     * set to the id of the current activity. Defaults to an empty id.
     *
     * @see setActivity
     * @returns the id of the activity used in filtering.
     **/
    QString activity() const;

    /**
     * Set the id of the activity to use in filtering by activity.
     *
     * @see activity
     * @param activity An activity id.
     **/
    void setActivity(const QString &activity);

    /**
     * Whether tasks should be filtered by virtual desktop. Defaults to
     * @c false.
     *
     * Filtering by virtual desktop only happens if a virtual desktop
     * number greater than -1 is set, even if this returns @c true.
     *
     * @see setFilterByVirtualDesktop
     * @see setVirtualDesktop
     * @returns @c true if tasks should be filtered by virtual desktop.
     **/
    bool filterByVirtualDesktop() const;

    /**
     * Set whether tasks should be filtered by virtual desktop.
     *
     * Filtering by virtual desktop only happens if a virtual desktop
     * number is set, even if this is set to @c true.
     *
     * @see filterByVirtualDesktop
     * @see setVirtualDesktop
     * @param filter Whether tasks should be filtered by virtual desktop.
     **/
    void setFilterByVirtualDesktop(bool filter);

    /**
     * Whether tasks should be filtered by screen. Defaults to @c false.
     *
     * Filtering by screen only happens if a screen number is set, even
     * if this returns @c true.
     *
     * @see setFilterByScreen
     * @see setScreen
     * @returns @c true if tasks should be filtered by screen.
     **/
    bool filterByScreen() const;

    /**
     * Set whether tasks should be filtered by screen.
     *
     * Filtering by screen only happens if a screen number is set, even
     * if this is set to @c true.
     *
     * @see filterByScreen
     * @see setScreen
     * @param filter Whether tasks should be filtered by screen.
     **/
    void setFilterByScreen(bool filter);

    /**
     * Whether tasks should be filtered by activity. Defaults to @c false.
     *
     * Filtering by activity only happens if an activity id is set, even
     * if this returns @c true.
     *
     * @see setFilterByActivity
     * @see setActivity
     * @returns @ctrue if tasks should be filtered by activity.
     **/
    bool filterByActivity() const;

    /**
     * Set whether tasks should be filtered by activity. Defaults to
     * @c false.
     *
     * Filtering by virtual desktop only happens if an activity id is set,
     * even if this is set to @c true.
     *
     * @see filterByActivity
     * @see setActivity
     * @param filter Whether tasks should be filtered by activity.
     **/
    void setFilterByActivity(bool filter);

    /**
     * Whether non-minimized tasks should be filtered. Defaults to
     * @c false.
     *
     * @see setFilterNotMinimized
     * @returns @c true if non-minimized tasks should be filtered.
     **/
    bool filterNotMinimized() const;

    /**
     * Set whether non-minimized tasks should be filtered.
     *
     * @see filterNotMinimized
     * @param filter Whether non-minimized tasks should be filtered.
     **/
    void setFilterNotMinimized(bool filter);

    /**
     * Request activation of the task at the given index. Derived classes are
     * free to interpret the meaning of "activate" themselves depending on
     * the nature and state of the task, e.g. launch or raise a window task.
     *
     * @param index An index in this tasks model.
     **/
    void requestActivate(const QModelIndex &index);

    /**
     * Request an additional instance of the application backing the task
     * at the given index.
     *
     * @param index An index in this tasks model.
     **/
    void requestNewInstance(const QModelIndex &index);

    /**
     * Request the task at the given index be closed.
     *
     * @param index An index in this tasks model.
     **/
    void requestClose(const QModelIndex &index);

    /**
     * Request starting an interactive move for the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    void requestMove(const QModelIndex &index);

    /**
     * Request starting an interactive resize for the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be a
     * no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    void requestResize(const QModelIndex &index);

    /**
     * Request toggling the minimized state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    void requestToggleMinimized(const QModelIndex &index);

    /**
     * Request toggling the maximized state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    void requestToggleMaximized(const QModelIndex &index);

    /**
     * Request toggling the keep-above state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    void requestToggleKeepAbove(const QModelIndex &index);

    /**
     * Request toggling the keep-below state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    void requestToggleKeepBelow(const QModelIndex &index);

    /**
     * Request toggling the fullscreen state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    void requestToggleFullScreen(const QModelIndex &index);

    /**
     * Request toggling the shaded state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    void requestToggleShaded(const QModelIndex &index);

    /**
     * Request moving the task at the given index to the specified virtual
     * desktop.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     * @param desktop A virtual desktop number.
     **/
    void requestVirtualDesktop(const QModelIndex &index, qint32 desktop);

    /**
     * Request informing the window manager of new geometry for a visual
     * delegate for the task at the given index. The geometry should be in
     * screen coordinates.
     *
     * @param index An index in this tasks model.
     * @param geometry Visual delegate geometry in screen coordinates.
     * @param delegate The delegate. Implementations are on their own with
     * regard to extracting information from this, and should take care to
     * reject invalid objects.
     **/
    void requestPublishDelegateGeometry(const QModelIndex &index, const QRect &geometry,
        QObject *delegate = nullptr);

Q_SIGNALS:
    void virtualDesktopChanged() const;
    void screenChanged() const;
    void activityChanged() const;
    void filterByVirtualDesktopChanged() const;
    void filterByScreenChanged() const;
    void filterByActivityChanged() const;
    void filterNotMinimizedChanged() const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    class Private;
    QScopedPointer<Private> d;
};

}

#endif
