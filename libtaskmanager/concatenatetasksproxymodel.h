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

#ifndef CONCATENATETASKSPROXYMODEL_H
#define CONCATENATETASKSPROXYMODEL_H

#include "abstracttasksmodeliface.h"

#include <KConcatenateRowsProxyModel>

#include "taskmanager_export.h"

namespace TaskManager
{

/**
 * @short A proxy tasks model for concatenating multiple source tasks models.
 *
 * This proxy model is a subclass of KConcatenateRowsProxyModel implementing
 * AbstractTasksModelIface, forwarding calls to the correct source model.
 *
 * @author Eike Hein <hein@kde.org>
 **/

class TASKMANAGER_EXPORT ConcatenateTasksProxyModel : public KConcatenateRowsProxyModel,
    public AbstractTasksModelIface
{
    Q_OBJECT

public:
    explicit ConcatenateTasksProxyModel(QObject *parent = 0);
    virtual ~ConcatenateTasksProxyModel();

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
};

}

#endif
