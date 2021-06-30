/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QObject>

#include "abstracttasksmodeliface.h"
#include "taskmanager_export.h"

namespace TaskManager
{
/**
 * @short Pure method interface for tasks model implementations.
 *
 * This is the pure method interface implemented by AbstractTasksModel,
 * as well as other model classes in this library which cannot inherit from
 * AbstractTasksModel.
 *
 * @author Eike Hein <hein@kde.org>
 **/

class TASKMANAGER_EXPORT AbstractTasksProxyModelIface : public AbstractTasksModelIface
{
public:
    ~AbstractTasksProxyModelIface() override
    {
    }

    /**
     * Request activation of the task at the given index. Implementing classes
     * are free to interpret the meaning of "activate" themselves depending on
     * the nature and state of the task, e.g. launch or raise a window task.
     *
     * @param index An index in this tasks model.
     **/
    void requestActivate(const QModelIndex &index) override;

    /**
     * Request an additional instance of the application backing the task
     * at the given index.
     *
     * @param index An index in this tasks model.
     **/
    void requestNewInstance(const QModelIndex &index) override;

    /**
     * Requests to open the given URLs with the application backing the task
     * at the given index.
     *
     * @param index An index in this tasks model.
     * @param urls The URLs to be passed to the application.
     **/
    void requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls) override;

    /**
     * Request the task at the given index be closed.
     *
     * @param index An index in this tasks model.
     **/
    void requestClose(const QModelIndex &index) override;

    /**
     * Request starting an interactive move for the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    void requestMove(const QModelIndex &index) override;

    /**
     * Request starting an interactive resize for the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be a
     * no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    void requestResize(const QModelIndex &index) override;

    /**
     * Request toggling the minimized state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    void requestToggleMinimized(const QModelIndex &index) override;

    /**
     * Request toggling the maximized state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    void requestToggleMaximized(const QModelIndex &index) override;

    /**
     * Request toggling the keep-above state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    void requestToggleKeepAbove(const QModelIndex &index) override;

    /**
     * Request toggling the keep-below state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    void requestToggleKeepBelow(const QModelIndex &index) override;

    /**
     * Request toggling the fullscreen state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    void requestToggleFullScreen(const QModelIndex &index) override;

    /**
     * Request toggling the shaded state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     **/
    void requestToggleShaded(const QModelIndex &index) override;

    /**
     * Request entering the window at the given index on the specified virtual desktops,
     * leaving any other desktops.
     *
     * On Wayland, virtual desktop ids are QStrings. On X11, they are uint >0.
     *
     * An empty list has a special meaning: The window is entered on all virtual desktops
     * in the session.
     *
     * On X11, a window can only be on one or all virtual desktops. Therefore, only the
     * first list entry is actually used.
     *
     * On X11, the id 0 has a special meaning: The window is entered on all virtual
     * desktops in the session.
     *
     * @param index An index in this window tasks model.
     * @param desktops A list of virtual desktop ids.
     **/
    void requestVirtualDesktops(const QModelIndex &index, const QVariantList &desktops) override;

    /**
     * Request entering the window at the given index on a new virtual desktop,
     * which is created in response to this request.
     *
     * @param index An index in this window tasks model.
     **/
    void requestNewVirtualDesktop(const QModelIndex &index) override;

    /**
     * Request moving the task at the given index to the specified activities.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * @param index An index in this tasks model.
     * @param activities The new list of activities.
     **/
    void requestActivities(const QModelIndex &index, const QStringList &activities) override;

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
    void requestPublishDelegateGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate = nullptr) override;

protected:
    /*
     * Map the passed QModelIndex to the source model index
     * so that AbstractTasksModelIface methods can be passed on
     * Subclasses should override this.
     */
    virtual QModelIndex mapIfaceToSource(const QModelIndex &index) const = 0;
};

}
