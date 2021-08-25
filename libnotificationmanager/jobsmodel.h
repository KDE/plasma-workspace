/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QAbstractListModel>
#include <QScopedPointer>
#include <QSharedPointer>

#include "notificationmanager_export.h"
#include "notifications.h"

namespace NotificationManager
{
class JobsModelPrivate;

/**
 * A model used for listing Job.
 */
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

Q_SIGNALS:
    void serviceOwnershipLost();

private:
    JobsModel();
    Q_DISABLE_COPY(JobsModel)

    JobsModelPrivate *d;
};

} // namespace NotificationManager
