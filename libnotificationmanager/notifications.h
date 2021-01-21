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

#include <QQmlParserStatus>
#include <QSortFilterProxyModel>

#include <QScopedPointer>

#include "notificationmanager_export.h"

namespace NotificationManager
{
/**
 * @brief A model with notifications and jobs
 *
 * This model contains application notifications as well as jobs
 * and lets you apply fine-grained filter, sorting, and grouping rules.
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
     * Whether to show notifications.
     *
     * Default is true.
     */
    Q_PROPERTY(bool showNotifications READ showNotifications WRITE setShowNotifications NOTIFY showNotificationsChanged)

    /**
     * Whether to show application jobs.
     *
     * Default is false.
     */
    Q_PROPERTY(bool showJobs READ showJobs WRITE setShowJobs NOTIFY showJobsChanged)

    /**
     * The notification urgency types the model should contain.
     *
     * Default is all urgencies: low, normal, critical.
     */
    Q_PROPERTY(Urgencies urgencies READ urgencies WRITE setUrgencies NOTIFY urgenciesChanged)

    /**
     * The sort mode for notifications.
     *
     * Default is strictly by date created/updated.
     */
    Q_PROPERTY(SortMode sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)

    /**
     * The sort order for notifications.
     *
     * This only affects the sort order by date. When @c sortMode is set to SortByTypeAndUrgency
     * the order of notification groups (e.g. high - jobs - normal - low) is unaffected, and only
     * notifications within the same group are either sorted ascending or descending by their
     * creation/update date.
     *
     * Default is DescendingOrder, i.e. newest notifications come first.
     *
     * @since 5.19
     */
    Q_PROPERTY(Qt::SortOrder sortOrder READ sortOrder WRITE setSortOrder NOTIFY sortOrderChanged)

    /**
     * The group mode for notifications.
     *
     * Default is ungrouped.
     */
    Q_PROPERTY(GroupMode groupMode READ groupMode WRITE setGroupMode NOTIFY groupModeChanged)

    /**
     * How many notifications are shown in each group.
     *
     * You can expand a group by setting the IsGroupExpandedRole to true.
     *
     * Default is 0, which means no limit.
     */
    Q_PROPERTY(int groupLimit READ groupLimit WRITE setGroupLimit NOTIFY groupLimitChanged)

    /**
     * Whether to automatically show notifications that are unread.
     *
     * This is any notification that was created or updated after the value of @c lastRead.
     */
    Q_PROPERTY(bool expandUnread READ expandUnread WRITE setExpandUnread NOTIFY expandUnreadChanged)

    /**
     * The number of notifications in the model
     */
    Q_PROPERTY(int count READ count NOTIFY countChanged)

    /**
     * The number of active, i.e. non-expired notifications
     */
    Q_PROPERTY(int activeNotificationsCount READ activeNotificationsCount NOTIFY activeNotificationsCountChanged)

    /**
     * The number of inactive, i.e. non-expired notifications
     */
    Q_PROPERTY(int expiredNotificationsCount READ expiredNotificationsCount NOTIFY expiredNotificationsCountChanged)

    /**
     * The time when the user last could read the notifications.
     * This is typically reset whenever the list of notifications is opened and is used to determine
     * the @c unreadNotificationsCount
     */
    Q_PROPERTY(QDateTime lastRead READ lastRead WRITE setLastRead RESET resetLastRead NOTIFY lastReadChanged)

    /**
     * The number of notifications added since lastRead
     *
     * This can be used to show a "n unread notifications" label
     */
    Q_PROPERTY(int unreadNotificationsCount READ unreadNotificationsCount NOTIFY unreadNotificationsCountChanged)

    /**
     * The number of active jobs
     */
    Q_PROPERTY(int activeJobsCount READ activeJobsCount NOTIFY activeJobsCountChanged)
    /**
     * The combined percentage of all jobs.
     *
     * This is the average of all percentages and could can be used to show
     * a global progress bar.
     */
    Q_PROPERTY(int jobsPercentage READ jobsPercentage NOTIFY jobsPercentageChanged)

public:
    explicit Notifications(QObject *parent = nullptr);
    ~Notifications() override;

    enum Roles {
        IdRole = Qt::UserRole + 1, ///< A notification identifier. This can be uint notification ID or string application job source.
        SummaryRole = Qt::DisplayRole, ///< The notification summary.
        ImageRole = Qt::DecorationRole, ///< The notification main image, which is not the application icon. Only valid for pixmap icons.

        IsGroupRole = Qt::UserRole + 2, ///< Whether the item is a group
        GroupChildrenCountRole, ///< The number of children in a group.
        ExpandedGroupChildrenCountRole, ///< The number of children in a group that are expanded.
        IsGroupExpandedRole, ///< Whether the group is expanded, this role is writable.

        IsInGroupRole, ///< Whether the notification is currently inside a group.
        TypeRole, ///< The type of model entry, either NotificationType or JobType.
        CreatedRole, ///< When the notification was first created.
        UpdatedRole, ///< When the notification was last updated, invalid when it hasn't been updated.

        BodyRole, ///< The notification body text.
        IconNameRole, ///< The notification main icon name, which is not the application icon. Only valid for icon names, if a URL supplied, it is loaded and
                      ///< exposed as ImageRole instead.

        DesktopEntryRole, ///< The desktop entry (without .desktop suffix, e.g. org.kde.spectacle) of the application that sent the notification.
        NotifyRcNameRole, ///< The notifyrc name (e.g. spectaclerc) of the application that sent the notification.

        ApplicationNameRole, ///< The user-visible name of the application (e.g. Spectacle)
        ApplicationIconNameRole, ///< The icon name of the application
        OriginNameRole, ///< The name of the device or account the notification originally came from, e.g. "My Phone" (in case of device sync) or
                        ///< "foo@example.com" (in case of an email notification)

        // Jobs
        JobStateRole, ///< The state of the job, either JobStateJopped, JobStateSuspended, or JobStateRunning.
        PercentageRole, ///< The percentage of the job. Use @c jobsPercentage to get a global percentage for all jobs.
        JobErrorRole, ///< The error id of the job, zero in case of no error.
        SuspendableRole, ///< Whether the job can be suspended @sa suspendJob
        KillableRole, ///< Whether the job can be killed/canceled @sa killJob
        JobDetailsRole, ///< A pointer to a Job item itself containing more detailed information about the job

        ActionNamesRole, ///< The IDs of the actions, excluding the default and settings action, e.g. [action1, action2]
        ActionLabelsRole, ///< The user-visible labels of the actions, excluding the default and settings action, e.g. ["Accept", "Reject"]
        HasDefaultActionRole, ///< Whether the notification has a default action, which is one that is invoked when the popup itself is clicked
        DefaultActionLabelRole, ///< The user-visible label of the default action, typically not shown as the popup itself becomes clickable

        UrlsRole, ///< A list of URLs associated with the notification, e.g. a path to a screenshot that was just taken or image received

        UrgencyRole, ///< The notification urgency, either LowUrgency, NormalUrgency, or CriticalUrgency. Jobs do not have an urgency.
        TimeoutRole, ///< The timeout for the notification in milliseconds. 0 means the notification should not timeout, -1 means a sensible default should be
                     ///< applied.

        ConfigurableRole, ///< Whether the notification can be configured because a desktopEntry or notifyRcName is known, or the notification has a setting
                          ///< action. @sa configure
        ConfigureActionLabelRole, ///< The user-visible label for the settings action
        ClosableRole, ///< Whether the item can be closed. Notifications are always closable, jobs are only when in JobStateStopped.

        ExpiredRole, ///< The notification timed out and closed. Actions on it cannot be invoked anymore.
        DismissedRole, ///< The notification got temporarily hidden by the user but could still be interacted with.
        ReadRole, ///< Whether the notification got read by the user. If true, the notification isn't considered unread even if created after lastRead.
                  ///< @since 5.17

        UserActionFeedbackRole, ///< Whether this notification is a response/confirmation to an explicit user action. @since 5.18

        HasReplyActionRole, ///< Whether the notification has a reply action. @since 5.18
        ReplyActionLabelRole, ///< The user-visible label for the reply action. @since 5.18
        ReplyPlaceholderTextRole, ///< A custom placeholder text for the reply action, e.g. "Reply to Max...". @since 5.18
        ReplySubmitButtonTextRole, ///< A custom text for the reply submit button, e.g. "Submit Comment". @since 5.18
        ReplySubmitButtonIconNameRole, ///< A custom icon name for the reply submit button. @since 5.18
        CategoryRole, ///< The (optional) category of the notification. Notifications can optionally have a type indicator. Although neither client or nor
                      ///< server must support this, some may choose to. Those servers implementing categories may use them to intelligently display the
                      ///< notification in a certain way, or group notifications of similar types.  @since 5.21
    };
    Q_ENUM(Roles)

    /**
     * The type of model item.
     */
    enum Type {
        NoType,
        NotificationType, ///< This item represents a notification.
        JobType, ///< This item represents an application job.
    };
    Q_ENUM(Type)

    /**
     * The notification urgency.
     *
     * @note jobs do not have an urgency, yet still might be above normal urgency notifications.
     */
    enum Urgency {
        // these don't match the spec's value
        LowUrgency = 1 << 0, ///< The notification has low urgency, it is not important and may not be shown or added to a history.
        NormalUrgency = 1 << 1, ///< The notification has normal urgency. This is also the default if no urgecny is supplied.
        CriticalUrgency = 1 << 2,
    };
    Q_ENUM(Urgency)
    Q_DECLARE_FLAGS(Urgencies, Urgency)
    Q_FLAG(Urgencies)

    /**
     * Which items should be cleared in a call to @c clear
     */
    enum ClearFlag {
        ClearExpired = 1 << 1,
        // TODO more
    };
    Q_ENUM(ClearFlag)
    Q_DECLARE_FLAGS(ClearFlags, ClearFlag)
    Q_FLAG(ClearFlags)

    /**
     * The state an application job is in.
     */
    enum JobState {
        JobStateStopped, ///< The job is stopped. It has either finished (error is 0) or failed (error is not 0)
        JobStateRunning, ///< The job is currently running.
        JobStateSuspended, ///< The job is currentl paused
    };
    Q_ENUM(JobState)

    /**
     * The sort mode for the model.
     */
    enum SortMode {
        SortByDate = 0, ///< Sort notifications strictly by the date they were updated or created.
        // should this be flags? SortJobsFirst | SortByUrgency | ...?
        SortByTypeAndUrgency, ///< Sort notifications taking into account their type and urgency. The order is (descending): Critical, jobs, Normal, Low.
    };
    Q_ENUM(SortMode)

    /**
     * The group mode for the model.
     */
    enum GroupMode {
        GroupDisabled = 0,
        // GroupApplicationsTree, // TODO make actual tree
        GroupApplicationsFlat,
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

    bool showNotifications() const;
    void setShowNotifications(bool showNotifications);

    bool showJobs() const;
    void setShowJobs(bool showJobs);

    Urgencies urgencies() const;
    void setUrgencies(Urgencies urgencies);

    SortMode sortMode() const;
    void setSortMode(SortMode sortMode);

    Qt::SortOrder sortOrder() const;
    void setSortOrder(Qt::SortOrder sortOrder);

    GroupMode groupMode() const;
    void setGroupMode(GroupMode groupMode);

    int groupLimit() const;
    void setGroupLimit(int limit);

    bool expandUnread() const;
    void setExpandUnread(bool expand);

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
     * Convert the given QModelIndex into a QPersistentModelIndex
     */
    Q_INVOKABLE QPersistentModelIndex makePersistentModelIndex(const QModelIndex &idx) const;

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
    /**
     * @brief Configure a notification
     *
     * This will invoke the settings action, if available, otherwise open the
     * kcm_notifications KCM for configuring the respective application and event.
     */
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

    /**
     * @brief Reply to a notification
     *
     * Replies to the given notification with the given text.
     * @since 5.18
     */
    Q_INVOKABLE void reply(const QModelIndex &idx, const QString &text);

    /**
     * @brief Start automatic timeout of notifications
     *
     * Call this if you no longer handle the timeout yourself.
     *
     * @sa stopTimeout
     */
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

    /**
     * @brief Suspend a job
     */
    Q_INVOKABLE void suspendJob(const QModelIndex &idx);
    /**
     * @brief Resume a job
     */
    Q_INVOKABLE void resumeJob(const QModelIndex &idx);
    /**
     * @brief Kill a job
     */
    Q_INVOKABLE void killJob(const QModelIndex &idx);

    /**
     * @brief Clear notifications
     *
     * Removes the notifications matching th ClearFlags from the model.
     * This can be used for e.g. a "Clear History" action.
     */
    Q_INVOKABLE void clear(ClearFlags flags);

    /**
     * Returns a model index pointing to the group of a notification.
     */
    Q_INVOKABLE QModelIndex groupIndex(const QModelIndex &idx) const;

    Q_INVOKABLE void collapseAllGroups();

    QVariant data(const QModelIndex &index, int role) const override;
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
    void showNotificationsChanged();
    void showJobsChanged();
    void urgenciesChanged();
    void sortModeChanged();
    void sortOrderChanged();
    void groupModeChanged();
    void groupLimitChanged();
    void expandUnreadChanged();
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
