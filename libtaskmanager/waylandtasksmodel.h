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

#ifndef WAYLANDTASKSMODEL_H
#define WAYLANDTASKSMODEL_H

#include "abstractwindowtasksmodel.h"

#include "taskmanager_export.h"

namespace KWayland {

namespace Client {
    class PlasmaWindowManagement;
    class Surface;
}

}

namespace TaskManager
{

/**
 * @short A tasks model for Wayland windows.
 *
 * This model presents tasks sourced from window data on the Wayland
 * server the host process is connected to.
 *
 * FIXME: Filtering by window type still needed.
 *
 * @see WindowTasksModel
 *
 * @author Eike Hein <hein@kde.org>
 */

class TASKMANAGER_EXPORT WaylandTasksModel : public AbstractWindowTasksModel
{
    Q_OBJECT

public:
    explicit WaylandTasksModel(QObject *parent = nullptr);
    ~WaylandTasksModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex	index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const override;

    /**
     * Request activation of the window at the given index.
     *
     * FIXME: Lacks transient handling of X Windows version.
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
     * FIXME: X Windows version has extra virtual desktop logic.
     *
     * @param index An index in this window tasks model.
     **/
    void requestMove(const QModelIndex &index) override;

    /**
     * Request starting an interactive move for the window at the given index.
     *
     * FIXME: X Windows version has extra virtual desktop logic.
     *
     * @param index An index in this window tasks model.
     **/
    void requestResize(const QModelIndex &index) override;

    /**
     * Request toggling the minimized state of the window at the given index.
     *
     * FIXME: X Windows version has extra virtual desktop logic.
     *
     * @param index An index in this window tasks model.
     **/
    void requestToggleMinimized(const QModelIndex &index) override;

    /**
     * Request toggling the maximized state of the task at the given index.
     *
     * FIXME: X Windows version has extra virtual desktop logic.
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
     * Request entering the window at the given index on the specified virtual desktops,
     * leaving any other desktops.
     *
     * Virtual desktop ids are QStrings.
     *
     * An empty list has a special meaning: The window is entered on all virtual desktops
     * in the session.
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
     * Request moving the window at the given index to the specified activities
     *
     * FIXME: This currently does nothing as activities is not implemented in kwin/kwayland
     *
     * @param index An index in this window tasks model.
     * @param desktop A virtual desktop number.
     **/
    void requestActivities(const QModelIndex &index, const QStringList &activities) override;

    /**
     * Request informing the window manager of new geometry for a visual
     * delegate for the window at the given index. The geometry is retrieved
     * from the delegate object passed. Right now, QQuickItem is the only
     * supported delegate object type.
     *
     * FIXME: This introduces the dependency on Qt5::Quick. I might prefer
     * reversing this and publishing the window pointer through the model,
     * then calling PlasmaWindow::setMinimizeGeometry in the applet backend,
     * rather than hand delegate items into the lib, keeping the lib more UI-
     * agnostic.
     *
     * @param index An index in this window tasks model.
     * @param geometry Visual delegate geometry in screen coordinates. Unused
     * in this implementation.
     * @param delegate The delegate. This implementation will attempt to cast
     * it to QQuickItem, map its coordinates to its window and find the Wayland
     * Surface for the window.
     **/
    void requestPublishDelegateGeometry(const QModelIndex &index, const QRect &geometry,
        QObject *delegate = nullptr) override;

    /**
     * Tries to extract a process-internal Wayland window id from supplied mime data.
     *
     * @param mimeData Some mime data.
     * @param @ok Set to true or false on success or failure.
     */
    static quint32 winIdFromMimeData(const QMimeData *mimeData, bool *ok = nullptr);

    /**
     * Tries to extract process-internal Wayland window ids from supplied mime data.
     *
     * @param mimeData Some mime data.
     * @param @ok Set to true or false on success or failure.
     */
    static QList<quint32> winIdsFromMimeData(const QMimeData *mimeData, bool *ok = nullptr);

private:
    class Private;
    QScopedPointer<Private> d;
};

}

#endif
