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

#include <QSortFilterProxyModel>
#include <QQmlParserStatus>

#include <QScopedPointer>

#include "notificationmanager_export.h"

namespace NotificationManager
{

/**
 * @short TODO
 *
 * TODO
 *
 * @author Kai Uwe Broulik <kde@privat.broulik.de>
 **/
class NOTIFICATIONMANAGER_EXPORT Notifications : public QSortFilterProxyModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    /**
     * The number of notifications the model should at most contain.
     *
     * Default is 0, which is no limit.
     */
    Q_PROPERTY(int limit READ limit WRITE setLimit NOTIFY limitChanged)

    /**
     * Whether to show expired notifications.
     *
     * Expired notifications are those that timed out, ie. ones that were not explicitly
     * closed or acted upon by the user, nor revoked by the issuing application.
     *
     * An expired notification has its actions removed.
     *
     * Default is false.
     */
    Q_PROPERTY(bool showExpired READ showExpired WRITE setShowExpired NOTIFY showExpiredChanged)

    /**
     * Whether to show dismissed notifications.
     *
     * Dismissed notifications are those that are temporarily hidden by the user.
     * This can e.g. be a copy job that has its popup closed but still continues in the background.
     *
     * Default is false.
     */
    Q_PROPERTY(bool showDismissed READ showDismissed WRITE setShowDismissed NOTIFY showDismissedChanged)

    /**
     * Whether to show suppressed notifications.
     *
     * Supressed notifications are those that should not be shown to the user owing to
     * notification settings or Do Not Disturb mode.
     */
    Q_PROPERTY(bool showSuppressed READ showSuppressed WRITE setShowSuppressed NOTIFY showSuppressedChanged)

    /**
     * Whether to show application jobs.
     */
    Q_PROPERTY(bool showJobs READ showJobs WRITE setShowJobs NOTIFY showJobsChanged)

    /**
     * The notification urgency types the model should contain.
     *
     * Default is all urgencies: low, normal, critical.
     */
    Q_PROPERTY(Urgencies urgencies READ urgencies WRITE setUrgencies NOTIFY urgenciesChanged)

    // TODO
    Q_PROPERTY(GroupMode groupMode READ groupMode WRITE setGroupMode NOTIFY groupModeChanged)

    /**
     * The number of notifications in the model
     */
    // FIXME not implemented I think
    Q_PROPERTY(int count READ count NOTIFY countChanged)

    /**
     * Whether there is an active, ie. non-expired notification currently in the model
     */
    Q_PROPERTY(int activeNotificationsCount READ activeNotificationsCount NOTIFY activeNotificationsCountChanged)

    // TODO what about "dismissed" jobs?
    Q_PROPERTY(int expiredNotificationsCount READ expiredNotificationsCount NOTIFY expiredNotificationsCountChanged)

    // TODO do we need an dismissed or suppressed notifications count?

    Q_PROPERTY(int activeJobsCount READ activeJobsCount NOTIFY activeJobsCountChanged)
    Q_PROPERTY(int jobsPercentage READ jobsPercentage NOTIFY jobsPercentageChanged)

public:
    explicit Notifications(QObject *parent = nullptr);
    ~Notifications() override;

    enum Roles {
        // TODO we have uint for notifications and source name string for jobs
        IdRole = Qt::UserRole + 1,
        TypeRole,
        CreatedRole,
        UpdatedRole,
        SummaryRole = Qt::DisplayRole,
        BodyRole = Qt::UserRole + 5,
        IconNameRole = Qt::UserRole + 6,
        ImageRole = Qt::DecorationRole,
        ApplicationNameRole = Qt::UserRole + 7,
        ApplicationIconNameRole,

        // Jobs
        JobStateRole,
        PercentageRole,
        ErrorRole,
        ErrorTextRole,
        SuspendableRole,
        KillableRole,
        JobDetailsRole,

        ActionNamesRole,
        ActionLabelsRole,
        HasDefaultActionRole,

        UrlsRole,
        UrgencyRole,
        TimeoutRole,

        ConfigurableRole,
        ConfigureActionLabelRole,

        /**
         * The notification timed out and closed.
         * Actions on it cannot be invoked anymore
         */
        ExpiredRole,
        /**
         * The notificaton got dismissed by the user or not presented
         * because of Do Not Disturb settings, but can still be
         * listed in e.g. a missed notifications list.
         */
        DismissedRole
        // Ignored/SuppressedRole?
    };
    Q_ENUM(Roles) // for qDebug TODO remove?

    enum Type {
        NoType,
        NotificationType,
        JobType
    };
    Q_ENUM(Type)

    enum Urgency {
        // these don't match the spec's value
        LowUrgency = 1 << 0,
        NormalUrgency = 1 << 1,
        CriticalUrgency = 1 << 2
    };
    Q_ENUM(Urgency)
    Q_DECLARE_FLAGS(Urgencies, Urgency)
    Q_FLAG(Urgencies)

    enum JobState {
        JobStateStopped,
        JobStateRunning,
        JobStateSuspended
    };
    Q_ENUM(JobState)

    enum GroupMode {
        GroupDisabled = 0,
        GroupApplications
    };
    Q_ENUM(GroupMode)

    int limit() const;
    void setLimit(int limit);

    bool showExpired() const;
    void setShowExpired(bool show);

    bool showDismissed() const;
    void setShowDismissed(bool show);

    bool showSuppressed() const;
    void setShowSuppressed(bool show);

    bool showJobs() const;
    void setShowJobs(bool showJobs);

    Urgencies urgencies() const;
    void setUrgencies(Urgencies urgencies);

    GroupMode groupMode() const;
    void setGroupMode(GroupMode groupMode);

    int count() const;

    int activeNotificationsCount() const;
    int expiredNotificationsCount() const;
    int activeJobsCount() const;
    int jobsPercentage() const;

    Q_INVOKABLE QModelIndex makeModelIndex(int row, int childRow = -1) const;

    Q_INVOKABLE void expire(const QModelIndex &idx);
    Q_INVOKABLE void dismiss(const QModelIndex &idx);
    Q_INVOKABLE void close(const QModelIndex &idx);
    Q_INVOKABLE void configure(const QModelIndex &idx); // TODO pass ctx for transient handling
    Q_INVOKABLE void invokeDefaultAction(const QModelIndex &idx);
    Q_INVOKABLE void invokeAction(const QModelIndex &idx, const QString &actionId);

    Q_INVOKABLE void suspendJob(const QModelIndex &idx);
    Q_INVOKABLE void resumeJob(const QModelIndex &idx);
    Q_INVOKABLE void killJob(const QModelIndex &idx);

    QVariant data(const QModelIndex &index, int role/* = Qt::DisplayRole*/) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

signals:
    void limitChanged();
    void showExpiredChanged();
    void showDismissedChanged();
    void showSuppressedChanged();
    void showJobsChanged();
    void urgenciesChanged();
    void groupModeChanged();
    void countChanged();
    void activeNotificationsCountChanged();
    void expiredNotificationsCountChanged();
    void activeJobsCountChanged();
    void jobsPercentageChanged();

protected:
    void classBegin() override;
    void componentComplete() override;

private:
    class Private;
    QScopedPointer<Private> d;

};

} // namespace NotificationManager

Q_DECLARE_OPERATORS_FOR_FLAGS(NotificationManager::Notifications::Urgencies)
