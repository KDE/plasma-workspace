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

#ifndef WINDOWTASKSMODEL_H
#define WINDOWTASKSMODEL_H

#include <QIdentityProxyModel>

#include "abstracttasksmodeliface.h"

#include "taskmanager_export.h"

namespace TaskManager
{

/**
 * @short A window tasks model.
 *
 * This model presents tasks sourced from window data retrieved from the
 * windowing server the host process is connected to. The underlying
 * windowing system is abstracted away.
 *
 * @author Eike Hein <hein@kde.org>
 **/

class TASKMANAGER_EXPORT WindowTasksModel : public QIdentityProxyModel, public AbstractTasksModelIface
{
    Q_OBJECT

public:
    explicit WindowTasksModel(QObject *parent = 0);
    virtual ~WindowTasksModel();

    QHash<int, QByteArray> roleNames() const override;

    /**
     * Request activation of the window at the given index.
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
     * Request the window at the given index be closed.
     *
     * @param index An index in this window tasks model.
     **/
    void requestClose(const QModelIndex &index) override;

    /**
     * Request starting an interactive move for the window at the given index.
     *
     * @param index An index in this window tasks model.
     **/
    void requestMove(const QModelIndex &index) override;

    /**
     * Request starting an interactive resize for the window at the given index.
     *
     * @param index An index in this window tasks model.
     **/
    void requestResize(const QModelIndex &index) override;

    /**
     * Request toggling the minimized state of the window at the given index.
     *
     * @param index An index in this window tasks model.
     **/
    void requestToggleMinimized(const QModelIndex &index) override;

    /**
     * Request toggling the maximized state of the window at the given index.
     *
     * @param index An index in this window tasks model.
     **/
    void requestToggleMaximized(const QModelIndex &index) override;

    /**
     * Request toggling the keep-above state of the window at the given index.
     *
     * @param index An index in this window tasks model.
     **/
    void requestToggleKeepAbove(const QModelIndex &index) override;

    /**
     * Request toggling the keep-below state of the window at the given index.
     *
     * @param index An index in this window tasks model.
     **/
    void requestToggleKeepBelow(const QModelIndex &index) override;

    /**
     * Request toggling the fullscreen state of the window at the given index.
     *
     * @param index An index in this window tasks model.
     **/
    void requestToggleFullScreen(const QModelIndex &index) override;

    /**
     * Request toggling the shaded state of the window at the given index.
     *
     * @param index An index in this window tasks model.
     **/
    void requestToggleShaded(const QModelIndex &index) override;

    /**
     * Request moving the window at the given index to the specified virtual
     * desktop.
     *
     * @param index An index in this window tasks model.
     * @param desktop A virtual desktop number.
     **/
    void requestVirtualDesktop(const QModelIndex &index, qint32 desktop) override;

    /**
     * Request moving the window at the given index to the specified activities.
     *
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
     * @param delegate The delegate.
     **/
    void requestPublishDelegateGeometry(const QModelIndex &index, const QRect &geometry,
        QObject *delegate = nullptr) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

}

#endif
