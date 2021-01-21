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

#ifndef XWINDOWTASKSMODEL_H
#define XWINDOWTASKSMODEL_H

#include "abstractwindowtasksmodel.h"

#include "taskmanager_export.h"

#include <QMimeData>
#include <qwindowdefs.h>

#include <netwm.h>

namespace TaskManager
{
/**
 * @short A tasks model for X Window System windows.
 *
 * This model presents tasks sourced from window data on the X Windows
 * server the host process is connected to.
 *
 * For the purposes of presentation in a user interface and efficiency,
 * certain types of windows (e.g. utility windows, or windows that are
 * transients for an otherwise-included window) are omitted from the
 * model.
 *
 * @see WindowTasksModel
 *
 * @author Eike Hein <hein@kde.org>
 */

class TASKMANAGER_EXPORT XWindowTasksModel : public AbstractWindowTasksModel
{
    Q_OBJECT

public:
    explicit XWindowTasksModel(QObject *parent = nullptr);
    ~XWindowTasksModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * Request activation of the window at the given index.
     *
     * If the window has a transient demanding attention, it will be
     * activated instead.
     *
     * If the window has a transient in shaded state, it will be
     * activated instead.
     *
     * @param index An index in this window tasks model.
     **/
    void requestActivate(const QModelIndex &index) override;

    /**
     * Request an additional instance of the application owning the window
     * at the given index. Success depends on whether a
     * AbstractTasksModel::LauncherUrl could be derived from window metadata.
     *
     * @param index An index in this window tasks model.
     **/
    void requestNewInstance(const QModelIndex &index) override;

    /**
     * Runs the application backing the launcher at the given index with the given URLs.
     * Success depends on whether a AbstractTasksModel::LauncherUrl could be
     * derived from window metadata and a KService could be found from that.
     *
     * @param index An index in this launcher tasks model
     * @param urls The URLs to be passed to the application
     */
    void requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls) override;

    /**
     * Request the window at the given index be closed.
     *
     * @param index An index in this window tasks model.
     **/
    void requestClose(const QModelIndex &index) override;

    /**
     * Request starting an interactive move for the window at the given index.
     *
     * If the window is not currently the active window, it will be activated.
     *
     * If the window is not on the current desktop, the current desktop will
     * be set to the window's desktop.
     * FIXME: Desktop logic should maybe move into proxy.
     *
     * @param index An index in this window tasks model.
     **/
    void requestMove(const QModelIndex &index) override;

    /**
     * Request starting an interactive resize for the window at the given index.
     *
     * If the window is not currently the active window, it will be activated.
     *
     * If the window is not on the current desktop, the current desktop will
     * be set to the window's desktop.
     * FIXME: Desktop logic should maybe move into proxy.
     *
     * @param index An index in this window tasks model.
     **/
    void requestResize(const QModelIndex &index) override;

    /**
     * Request toggling the minimized state of the window at the given index.
     *
     * If the window is not on the current desktop, the current desktop will
     * be set to the window's desktop.
     * FIXME: Desktop logic should maybe move into proxy.
     *
     * @param index An index in this window tasks model.
     **/
    void requestToggleMinimized(const QModelIndex &index) override;

    /**
     * Request toggling the maximized state of the task at the given index.
     *
     * If the window is not on the current desktop, the current desktop will
     * be set to the window's desktop.
     * FIXME: Desktop logic should maybe move into proxy.
     *
     * @param index An index in this window tasks model.
     **/
    void requestToggleMaximized(const QModelIndex &index) override;

    /**
     * Request toggling the keep-above state of the task at the given index.
     *
     * @param index An index in this window tasks model.
     **/
    void requestToggleKeepAbove(const QModelIndex &index) override;

    /**
     * Request toggling the keep-below state of the task at the given index.
     *
     * @param index An index in this window tasks model.
     **/
    void requestToggleKeepBelow(const QModelIndex &index) override;

    /**
     * Request toggling the fullscreen state of the task at the given index.
     *
     * @param index An index in this window tasks model.
     **/
    void requestToggleFullScreen(const QModelIndex &index) override;

    /**
     * Request toggling the shaded state of the task at the given index.
     *
     * @param index An index in this window tasks model.
     **/
    void requestToggleShaded(const QModelIndex &index) override;

    /**
     * Request entering the window at the given index on the specified virtual desktop.
     * For compatibility across windowing systems the library supports, the desktops
     * parameter is a list; however, on X11 a window can only be on one or all virtual
     * desktops. Therefore, only the first list entry is actually used.
     *
     * An empty list has a special meaning: The window is entered on all virtual desktops
     * in the session.
     *
     * The id 0 has a special meaning: The window is entered on all virtual desktops in
     * the session.
     *
     * @param index An index in this window tasks model.
     * @param desktops A list of virtual desktop ids (uint).
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
     *     *
     * @param index An index in this tasks model.
     * @param activities The new list of activities.
     **/
    void requestActivities(const QModelIndex &index, const QStringList &activities) override;

    /**
     * Request informing the window manager of new geometry for a visual
     * delegate for the window at the given index.
     *
     * @param index An index in this window tasks model.
     * @param geometry Visual delegate geometry in screen coordinates.
     * @param delegate The delegate. Unused in this implementation.
     **/
    void requestPublishDelegateGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate = nullptr) override;

    /**
     * Tries to extract a X11 window id from supplied mime data.
     *
     * @param mimeData Some mime data.
     * @param @ok Set to true or false on success or failure.
     */
    static WId winIdFromMimeData(const QMimeData *mimeData, bool *ok = nullptr);

    /**
     * Tries to extract X11 window ids from supplied mime data.
     *
     * @param mimeData Some mime data.
     * @param @ok Set to true or false on success or failure.
     */
    static QList<WId> winIdsFromMimeData(const QMimeData *mimeData, bool *ok = nullptr);

private:
    class Private;
    QScopedPointer<Private> d;
};

}

#endif
