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

#ifndef STARTUPTASKSMODEL_H
#define STARTUPTASKSMODEL_H

#include "abstracttasksmodel.h"
#include "taskmanager_export.h"

namespace TaskManager
{

/**
 * @short A tasks model for startup notifications.
 *
 * This model presents tasks sourced from startup notifications.
 *
 * Startup notifications are given a timeout sourced from klaunchrc, falling
 * back to a default of 5 seconds if no configuration is present.
 *
 * Startup tasks are removed 500 msec after cancellation or timeout to
 * overlap with window tasks appearing in window task models.
 *
 * @author Eike Hein <hein@kde.org>
 */

class TASKMANAGER_EXPORT StartupTasksModel : public AbstractTasksModel
{
    Q_OBJECT

public:
    explicit StartupTasksModel(QObject *parent = nullptr);
    ~StartupTasksModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * Request an additional instance of the application backing the
     * startup notification at the given index.
     *
     * @param index An index in this startup tasks model.
     **/
    void requestNewInstance(const QModelIndex &index) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

}

#endif
