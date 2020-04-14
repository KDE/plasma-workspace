/*
 * Copyright 2019 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QAbstractListModel>
#include <QScopedPointer>
#include <QSharedPointer>

#include "notifications.h"
#include "notificationmanager_export.h"

namespace NotificationManager
{

class JobsModelPrivate;

class NOTIFICATIONMANAGER_EXPORT JobsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    ~JobsModel() override;

    using Ptr = QSharedPointer<JobsModel>;
    static Ptr createJobsModel();

    /**
     * Registers the JobView service on DBus.
     *
     * @return true if succeeded, false otherwise.
     */
    bool init();

    /**
     * Whether the notification service could be registered
     */
    bool isValid() const;

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    /**
     * @brief Close a job
     *
     * This removes the job from the model. This will not cancel the job!
     * Use @c kill if you want to cancel a job.
     */
    void close(const QModelIndex &idx);
    void expire(const QModelIndex &idx);

    /**
     * @brief Suspend a job
     */
    void suspend(const QModelIndex &idx);
    /**
     * @brief Resume a job
     */
    void resume(const QModelIndex &idx);
    /**
     * @brief Kill a job
     *
     * This cancels the job.
     */
    void kill(const QModelIndex &idx);

    void clear(Notifications::ClearFlags flags);

signals:
    void serviceOwnershipLost();

private:
    JobsModel();
    Q_DISABLE_COPY(JobsModel)

    JobsModelPrivate *d;

};

} // namespace NotificationManager
