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

#ifndef ABSTRACTTASKSMODEL_H
#define ABSTRACTTASKSMODEL_H

#include "abstracttasksmodeliface.h"

#include <QAbstractListModel>

#include "taskmanager_export.h"

namespace TaskManager
{

/**
 * @short An abstract base class for (flat) tasks models.
 *
 * This class serves as abstract base class for flat tasks model implementations.
 * It provides data roles and no-op default implementations of methods in the
 * AbstractTasksModelIface interface.
 *
 * @author Eike Hein <hein@kde.org>
 **/

class TASKMANAGER_EXPORT AbstractTasksModel : public QAbstractListModel,
    public AbstractTasksModelIface
{
    Q_OBJECT

public:
    enum AdditionalRoles {
        AppId = Qt::UserRole + 1, /**< KService storage id (.desktop name sans extension). */
        AppName,          /**< Application name. */
        GenericName,      /**< Generic application name. */
        LauncherUrl,      /**< URL that can be used to launch this application (.desktop or executable). */
        LauncherUrlWithoutIcon, /**< Special path to get a launcher URL while skipping fallback icon encoding. Used as speed optimization. */
        LegacyWinIdList,  /**< X11 window ids. Stopgap until we have something better. */
        MimeType,         /**< MIME type for this task (window, window group), needed for DND. */
        MimeData,         /**< Data for MimeType. */
        IsWindow,         /**< This is a window task. */
        IsStartup,        /**< This is a startup task. */
        IsLauncher,       /**< This is a launcher task. */
        IsGroupParent,    /**< This is a parent item for a group of child tasks. */
        ChildCount,       /**< The number of tasks in this group. */
        IsGroupable,      /**< Whether this task is being ignored by grouping or not. */
        IsActive,         /**< This is the currently active task. */
        IsClosable,       /**< requestClose (see below) available. */
        IsMovable,        /**< requestMove (see below) available. */
        IsResizable,      /**< requestResize (see below) available. */
        IsMaximizable,    /**< requestToggleMaximize (see below) available. */
        IsMaximized,      /**< Task (i.e. window) is maximized. */
        IsMinimizable,    /**< requestToggleMinimize (see below) available. */
        IsMinimized,      /**< Task (i.e. window) is minimized. */
        IsKeepAbove,      /**< Task (i.e. window) is keep-above. */
        IsKeepBelow,      /**< Task (i.e. window) is keep-below. */
        IsFullScreenable, /**< requestToggleFullScreen (see below) available. */
        IsFullScreen,     /**< Task (i.e. window) is fullscreen. */
        IsShadeable,      /**< requestToggleShade (see below) available. */
        IsShaded,         /**< Task (i.e. window) is shaded. */
        IsVirtualDesktopChangeable, /**< requestVirtualDesktop (see below) available. */
        VirtualDesktop,   /**< Virtual desktop for the task (i.e. window). */
        IsOnAllVirtualDesktops, /**< Task is on all virtual desktops. */
        Geometry,         /**< The task's geometry (i.e. the window's). */
        ScreenGeometry,   /**< Screen geometry for the task (i.e. the window's screen). */
        Activities,       /**< Activities for the task (i.e. window). */
        IsDemandingAttention, /**< Task is demanding attention. */
        SkipTaskbar,      /**< Task should not be shown in a 'task bar' user interface. */
        SkipPager,        /**< Task should not to be shown in a 'pager' user interface. */
    };
    Q_ENUM(AdditionalRoles)

    explicit AbstractTasksModel(QObject *parent = nullptr);
    virtual ~AbstractTasksModel();

    QHash<int, QByteArray> roleNames() const override;

    virtual QModelIndex	index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const override;

    /**
     * Request activation of the task at the given index. Derived classes are
     * free to interpret the meaning of "activate" themselves depending on
     * the nature and state of the task, e.g. launch or raise a window task.
     *
     * This base implementation does nothing.
     *
     * @param index An index in this tasks model.
     **/
    virtual void requestActivate(const QModelIndex &index) override;

    /**
     * Request an additional instance of the application backing the task
     * at the given index.
     *
     * This base implementation does nothing.
     *
     * @param index An index in this tasks model.
     **/
    virtual void requestNewInstance(const QModelIndex &index) override;

    /**
     * Requests to open the given URLs with the application backing the task
     * at the given index.
     *
     * This base implementation does nothing.
     *
     * @param index An index in this tasks model.
     * @param urls The URLs to be passed to the application.
     **/
    virtual void requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls) override;

    /**
     * Request the task at the given index be closed.
     *
     * This base implementation does nothing.
     *
     * @param index An index in this tasks model.
     **/
    virtual void requestClose(const QModelIndex &index) override;

    /**
     * Request starting an interactive move for the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * This base implementation does nothing.
     *
     * @param index An index in this tasks model.
     **/
    virtual void requestMove(const QModelIndex &index) override;

    /**
     * Request starting an interactive resize for the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be a
     * no-op when there is no window.
     *
     * This base implementation does nothing.
     *
     * @param index An index in this tasks model.
     **/
    virtual void requestResize(const QModelIndex &index) override;

    /**
     * Request toggling the minimized state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * This base implementation does nothing.
     *
     * @param index An index in this tasks model.
     **/
    virtual void requestToggleMinimized(const QModelIndex &index) override;

    /**
     * Request toggling the maximized state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * This base implementation does nothing.
     *
     * @param index An index in this tasks model.
     **/
    virtual void requestToggleMaximized(const QModelIndex &index) override;

    /**
     * Request toggling the keep-above state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * This base implementation does nothing.
     *
     * @param index An index in this tasks model.
     **/
    virtual void requestToggleKeepAbove(const QModelIndex &index) override;

    /**
     * Request toggling the keep-below state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * This base implementation does nothing.
     *
     * @param index An index in this tasks model.
     **/
    virtual void requestToggleKeepBelow(const QModelIndex &index) override;

    /**
     * Request toggling the fullscreen state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * This base implementation does nothing.
     *
     * @param index An index in this tasks model.
     **/
    virtual void requestToggleFullScreen(const QModelIndex &index) override;

    /**
     * Request toggling the shaded state of the task at the given index.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * This base implementation does nothing.
     *
     * @param index An index in this tasks model.
     **/
    virtual void requestToggleShaded(const QModelIndex &index) override;

    /**
     * Request moving the task at the given index to the specified virtual
     * desktop.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * This base implementation does nothing.
     *
     * @param index An index in this tasks model.
     * @param desktop A virtual desktop number.
     **/
    virtual void requestVirtualDesktop(const QModelIndex &index, qint32 desktop) override;

    /**
     * Request moving the task at the given index to the specified activities.
     *
     * This is meant for tasks that have an associated window, and may be
     * a no-op when there is no window.
     *
     * This base implementation does nothing.
     *
     * @param index An index in this tasks model.
     * @param activities The new list of activities.
     **/
    virtual void requestActivities(const QModelIndex &index, const QStringList &activities) override;

    /**
     * Request informing the window manager of new geometry for a visual
     * delegate for the task at the given index. The geometry should be in
     * screen coordinates.
     *
     * This base implementation does nothing.
     *
     * @param index An index in this tasks model.
     * @param geometry Visual delegate geometry in screen coordinates.
     * @param delegate The delegate. Implementations are on their own with
     * regard to extracting information from this, and should take care to
     * reject invalid objects.
     **/
    virtual void requestPublishDelegateGeometry(const QModelIndex &index, const QRect &geometry,
        QObject *delegate = nullptr) override;
};

}

#endif
