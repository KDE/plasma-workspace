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

#include "abstracttasksproxymodeliface.h"

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
 * @see WaylandTasksModel
 * @see XWindowTasksModel
 *
 * @author Eike Hein <hein@kde.org>
 **/

class TASKMANAGER_EXPORT WindowTasksModel : public QIdentityProxyModel, public AbstractTasksProxyModelIface
{
    Q_OBJECT

public:
    explicit WindowTasksModel(QObject *parent = nullptr);
    ~WindowTasksModel() override;

    QHash<int, QByteArray> roleNames() const override;

protected:
    QModelIndex mapIfaceToSource(const QModelIndex &index) const override;
private:
    class Private;
    QScopedPointer<Private> d;
};

}

#endif
