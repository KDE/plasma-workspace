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
     * Expired notifications are those that timed out, i.e. ones that were not explicitly
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
     * A list of desktop entries for which no notifications should be shown.
     *
     * If the same desktop entry is present in both blacklist and whitelist,
     * the blacklist takes precedence, i.e. the notification is not shown.
     */
    Q_PROPERTY(QStringList blacklistedDesktopEntries READ blacklistedDesktopEntries WRITE setBlacklistedDesktopEntries NOTIFY blacklistedDesktopEntriesChanged)

    /**
     * A list of notifyrc names for which no notifications should be shown.
     *
     * If the same notifyrc name is present in both blacklist and whitelist,
     * the blacklist takes precedence, i.e. the notification is not shown.
     */
    Q_PROPERTY(QStringList blacklistedNotifyRcNames READ blacklistedNotifyRcNames WRITE setBlacklistedNotifyRcNames NOTIFY blacklistedNotifyRcNamesChanged)

    /**
     * A list of desktop entries for which notifications should be shown.
     *
     * This bypasses any filtering for urgency.
     *
     * If the same desktop entry is present in both whitelist and blacklist,
     * the blacklist takes precedence, i.e. the notification is not shown.
     *
     * Default is empty list, which means normal filtering is applied.
     */
    Q_PROPERTY(QStringList whitelistedDesktopEntries READ whitelistedDesktopEntries WRITE setWhitelistedDesktopEntries NOTIFY whitelistedDesktopEntriesChanged)

    /**
     * A list of notifyrc names for which notifications should be shown.
     *
     * This bypasses any filtering for urgency.
     *
     * If the same notifyrc name is present in both whitelist and blacklist,
     * the blacklist takes precedence, i.e. the notification is not shown.
     *
     * Default is empty list, which means normal filtering is applied.
     */
    Q_PROPERTY(QStringList whitelistedNotifyRcNames READ whitelistedNotifyRcNames WRITE setWhitelistedNotifyRcNames NOTIFY whitelistedNotifyRcNamesChanged)

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

    Q_PROPERTY(SortMode sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)

    Q_PROPERTY(GroupMode groupMode READ groupMode WRITE setGroupMode NOTIFY groupModeChanged)

    Q_PROPERTY(int groupLimit READ groupLimit WRITE setGroupLimit NOTIFY groupLimitChanged)

    /**
     * The number of notifications in the model
     */
    Q_PROPERTY(int count READ count NOTIFY countChanged)

    /**
     * Whether there is an active, i.e. non-expired notification currently in the model
     */
    Q_PROPERTY(int activeNotificationsCount READ activeNotificationsCount NOTIFY activeNotificationsCountChanged)

    Q_PROPERTY(int expiredNotificationsCount READ expiredNotificationsCount NOTIFY expiredNotificationsCountChanged)

    /**
     * The time when the user last could read the notifications.
     * This is typically reset whenever the list of notifications is opened and is used to determine
     * the @c unreadNotificationsCount
     */
    Q_PROPERTY(QDateTime lastRead READ lastRead WRITE setLastRead RESET resetLastRead NOTIFY lastReadChanged)

    Q_PROPERTY(int unreadNotificationsCount READ unreadNotificationsCount NOTIFY unreadNotificationsCountChanged)

    Q_PROPERTY(int activeJobsCount READ activeJobsCount NOTIFY activeJobsCountChanged)
    Q_PROPERTY(int jobsPercentage READ jobsPercentage NOTIFY jobsPercentageChanged)

public:
    explicit Notifications(QObject *parent = nullptr);
    ~Notifications() override;

    enum Roles {
        IdRole = Qt::UserRole + 1, ///< A notification identifier. This can be uint notification ID or string application job source.
        SummaryRole = Qt::DisplayRole,
        ImageRole = Qt::DecorationRole,

        IsGroupRole = Qt::UserRole + 2,
        GroupChildrenCountRole,
        IsGroupExpandedRole,

        IsInGroupRole,
        TypeRole,
        CreatedRole,
        UpdatedRole,

        BodyRole,
        IconNameRole,

        DesktopEntryRole,
        NotifyRcNameRole,

        ApplicationNameRole,
        ApplicationIconNameRole,
        DeviceNameRole,

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
        DefaultActionLabelRole,

        UrlsRole,

        UrgencyRole,
        TimeoutRole,

        ConfigurableRole,
        ConfigureActionLabelRole,
        ClosableRole,

        ExpiredRole, ///< The notification timed out and closed. Actions on it cannot be invoked anymore.
        DismissedRole ///< The notification got temporarily hidden by the user but could still be interacted with.
    };

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

    enum ClearFlag {
        ClearExpired = 1 << 1,
        // TODO more
    };
    Q_ENUM(ClearFlag)
    Q_DECLARE_FLAGS(ClearFlags, ClearFlag)
    Q_FLAG(ClearFlags)

    enum JobState {
        JobStateStopped,
        JobStateRunning,
        JobStateSuspended
    };
    Q_ENUM(JobState)

    enum SortMode {
        SortByDate = 0,
        // should this be flags? SortJobsFirst | SortByUrgency | ...?
        SortByTypeAndUrgency
    };
    Q_ENUM(SortMode)

    enum GroupMode {
        GroupDisabled = 0,
        //GroupApplicationsTree, // TODO make actual tree
        GroupApplicationsFlat
    };
    Q_ENUM(GroupMode)

    int limit() const;
    void setLimit(int limit);

    bool showExpired() const;
    void setShowExpired(bool show);

    bool showDismissed() const;
    void setShowDismissed(bool show);

    QStringList blacklistedDesktopEntries() const;
    void setBlacklistedDesktopEntries(const QStringList &blacklist);

    QStringList blacklistedNotifyRcNames() const;
    void setBlacklistedNotifyRcNames(const QStringList &blacklist);

    QStringList whitelistedDesktopEntries() const;
    void setWhitelistedDesktopEntries(const QStringList &whitelist);

    QStringList whitelistedNotifyRcNames() const;
    void setWhitelistedNotifyRcNames(const QStringList &whitelist);

    bool showSuppressed() const;
    void setShowSuppressed(bool show);

    bool showJobs() const;
    void setShowJobs(bool showJobs);

    Urgencies urgencies() const;
    void setUrgencies(Urgencies urgencies);

    SortMode sortMode() const;
    void setSortMode(SortMode sortMode);

    GroupMode groupMode() const;
    void setGroupMode(GroupMode groupMode);

    int groupLimit() const;
    void setGroupLimit(int limit);

    int count() const;

    int activeNotificationsCount() const;
    int expiredNotificationsCount() const;

    QDateTime lastRead() const;
    void setLastRead(const QDateTime &lastRead);
    void resetLastRead();

    int unreadNotificationsCount() const;

    int activeJobsCount() const;
    int jobsPercentage() const;

    /**
     * @brief Expire a notification
     *
     * Closes the notification in response to its timeout running out.
     *
     * Call this if you have an implementation that handles the timeout itself
     * by having called @c stopTimeout
     *
     * @sa stopTimeout
     */
    Q_INVOKABLE void expire(const QModelIndex &idx);
    /**
     * @brief Close a notification
     *
     * Closes the notification in response to the user explicitly closing it.
     *
     * When the model index belongs to a group, the entire group is closed.
     */
    Q_INVOKABLE void close(const QModelIndex &idx);
    Q_INVOKABLE void configure(const QModelIndex &idx); // TODO pass ctx for transient handling
    /**
     * @brief Invoke the default notification action
     *
     * Invokes the action that should be triggered when clicking
     * the notification bubble itself.
     */
    Q_INVOKABLE void invokeDefaultAction(const QModelIndex &idx);
    /**
     * @brief Invoke a notification action
     *
     * Invokes the action with the given actionId on the notification.
     * For invoking the default action, i.e. the one that is triggered
     * when clicking the notification bubble, use invokeDefaultAction
     */
    Q_INVOKABLE void invokeAction(const QModelIndex &idx, const QString &actionId);

    Q_INVOKABLE void startTimeout(const QModelIndex &idx);
    Q_INVOKABLE void startTimeout(uint notificationId);
    /**
     * @brief Stop the automatic timeout of notifications
     *
     * Call this if you have an implementation that handles the timeout itself
     * taking into account e.g. whether the user is currently interacting with
     * the notification to not close it under their mouse. Call @c expire
     * once your custom timer has run out.
     *
     * @sa expire
     */
    Q_INVOKABLE void stopTimeout(const QModelIndex &idx);

    Q_INVOKABLE void suspendJob(const QModelIndex &idx);
    Q_INVOKABLE void resumeJob(const QModelIndex &idx);
    Q_INVOKABLE void killJob(const QModelIndex &idx);

    /**
     * @brief Clear notifications
     *
     * Removes the notifications matching th ClearFlags from the model.
     * This can be used for e.g. a "Clear History" action.
     */
    Q_INVOKABLE void clear(ClearFlags flags);

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
    void blacklistedDesktopEntriesChanged();
    void blacklistedNotifyRcNamesChanged();
    void whitelistedDesktopEntriesChanged();
    void whitelistedNotifyRcNamesChanged();
    void showSuppressedChanged();
    void showJobsChanged();
    void urgenciesChanged();
    void sortModeChanged();
    void groupModeChanged();
    void groupLimitChanged();
    void countChanged();
    void activeNotificationsCountChanged();
    void expiredNotificationsCountChanged();
    void lastReadChanged();
    void unreadNotificationsCountChanged();
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
