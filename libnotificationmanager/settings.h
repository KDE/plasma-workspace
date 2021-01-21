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

#include <QObject>

#include <KSharedConfig>

#include <QDateTime>
#include <QScopedPointer>
#include <QString>

#include "notificationmanager_export.h"

namespace NotificationManager
{
/**
 * @short Notification settings and state
 *
 * This class encapsulates all global settings related to notifications
 * as well as do not disturb mode and other state.
 *
 * This class can be used by applications to alter their behavior
 * depending on user's notification preferences.
 *
 * @author Kai Uwe Broulik <kde@privat.broulik.de>
 **/
class NOTIFICATIONMANAGER_EXPORT Settings : public QObject
{
    Q_OBJECT

    /**
     * Whether to show critical notification popups in do not disturb mode.
     */
    Q_PROPERTY(bool criticalPopupsInDoNotDisturbMode READ criticalPopupsInDoNotDisturbMode WRITE setCriticalPopupsInDoNotDisturbMode NOTIFY settingsChanged)
    /**
     * Whether to keep critical notifications always on top.
     */
    Q_PROPERTY(bool keepCriticalAlwaysOnTop READ keepCriticalAlwaysOnTop WRITE setKeepCriticalAlwaysOnTop NOTIFY settingsChanged)
    /**
     * Whether to show popups for low priority notifications.
     */
    Q_PROPERTY(bool lowPriorityPopups READ lowPriorityPopups WRITE setLowPriorityPopups NOTIFY settingsChanged)
    /**
     * Whether to add low priority notifications to the history.
     */
    Q_PROPERTY(bool lowPriorityHistory READ lowPriorityHistory WRITE setLowPriorityHistory NOTIFY settingsChanged)

    /**
     * The notification popup position on screen.
     * CloseToWidget means they should be positioned closely to where the plasmoid is located on screen.
     */
    Q_PROPERTY(PopupPosition popupPosition READ popupPosition WRITE setPopupPosition NOTIFY settingsChanged)

    /**
     * The default timeout for notification popups that do not have an explicit timeout set,
     * in milliseconds. Default is 5000ms (5 seconds).
     */
    Q_PROPERTY(int popupTimeout READ popupTimeout WRITE setPopupTimeout RESET resetPopupTimeout NOTIFY settingsChanged)

    /**
     * Whether to show application jobs in task manager
     */
    Q_PROPERTY(bool jobsInTaskManager READ jobsInTaskManager WRITE setJobsInTaskManager /*RESET resetJobsInTaskManager*/ NOTIFY settingsChanged)
    /**
     * Whether to show application jobs as notifications
     */
    Q_PROPERTY(bool jobsInNotifications READ jobsInNotifications WRITE setJobsInNotifications /*RESET resetJobsPopup*/ NOTIFY settingsChanged)
    /**
     * Whether application jobs stay visible for the whole duration of the job
     */
    Q_PROPERTY(bool permanentJobPopups READ permanentJobPopups WRITE setPermanentJobPopups /*RESET resetAutoHideJobsPopup*/ NOTIFY settingsChanged)

    /**
     * Whether to show notification badges (numbers in circles) in task manager
     */
    Q_PROPERTY(bool badgesInTaskManager READ badgesInTaskManager WRITE setBadgesInTaskManager NOTIFY settingsChanged)

    /**
     * A list of desktop entries of applications that have been seen sending a notification.
     */
    Q_PROPERTY(QStringList knownApplications READ knownApplications NOTIFY knownApplicationsChanged)

    /**
     * A list of desktop entries of applications for which no popups should be shown.
     */
    Q_PROPERTY(QStringList popupBlacklistedApplications READ popupBlacklistedApplications NOTIFY settingsChanged)
    /**
     * A list of notifyrc names of services for which no popups should be shown.
     */
    Q_PROPERTY(QStringList popupBlacklistedServices READ popupBlacklistedServices NOTIFY settingsChanged)

    /**
     * A list of desktop entries of applications for which a popup should be shown even in do not disturb mode.
     */
    Q_PROPERTY(QStringList doNotDisturbPopupWhitelistedApplications READ doNotDisturbPopupWhitelistedApplications NOTIFY settingsChanged)
    /**
     * A list of notifyrc names of services for which a popup should be shown even in do not disturb mode.
     */
    Q_PROPERTY(QStringList doNotDisturbPopupWhitelistedServices READ doNotDisturbPopupWhitelistedServices NOTIFY settingsChanged)

    /**
     * A list of desktop entries of applications which shouldn't be shown in the history.
     */
    Q_PROPERTY(QStringList historyBlacklistedApplications READ historyBlacklistedApplications NOTIFY settingsChanged)
    /**
     * A list of notifyrc names of services which shouldn't be shown in the history.
     */
    Q_PROPERTY(QStringList historyBlacklistedServices READ historyBlacklistedServices NOTIFY settingsChanged)

    /**
     * A list of desktop entries of applications which shouldn't show badges in task manager.
     */
    Q_PROPERTY(QStringList badgeBlacklistedApplications READ badgeBlacklistedApplications NOTIFY settingsChanged)

    /**
     * The date until which do not disturb mode is enabled.
     *
     * When invalid or in the past, do not disturb mode should be considered disabled.
     * Do not disturb mode is considered active when this property points to a date
     * in the future OR notificationsInhibitedByApplication is true.
     */
    Q_PROPERTY(QDateTime notificationsInhibitedUntil READ notificationsInhibitedUntil WRITE setNotificationsInhibitedUntil RESET
                   resetNotificationsInhibitedUntil NOTIFY settingsChanged)

    /**
     * Whether an application currently requested do not disturb mode.
     *
     * Do not disturb mode is considered active when this property is true OR
     * notificationsInhibitedUntil points to a date in the future.
     *
     * @sa revokeApplicationInhibitions
     */
    Q_PROPERTY(bool notificationsInhibitedByApplication READ notificationsInhibitedByApplication NOTIFY notificationsInhibitedByApplicationChanged)

    Q_PROPERTY(QStringList notificationInhibitionApplications READ notificationInhibitionApplications NOTIFY notificationInhibitionApplicationsChanged)

    Q_PROPERTY(QStringList notificationInhibitionReasons READ notificationInhibitionReasons NOTIFY notificationInhibitionApplicationsChanged)

    /**
     * Whether to enable do not disturb mode when screens are mirrored/overlapping
     *
     * @since 5.17
     */
    Q_PROPERTY(bool inhibitNotificationsWhenScreensMirrored READ inhibitNotificationsWhenScreensMirrored WRITE setInhibitNotificationsWhenScreensMirrored NOTIFY
                   settingsChanged)

    /**
     * Whether there currently are mirrored/overlapping screens
     *
     * This property is only updated when @c inhibitNotificationsWhenScreensMirrored
     * is set to true, otherwise it is always false.
     * You can assign false to this property if you want to temporarily revoke automatic do not disturb
     * mode when screens are mirrored until the screen configuration changes.
     *
     * @since 5.17
     */
    Q_PROPERTY(bool screensMirrored READ screensMirrored WRITE setScreensMirrored NOTIFY screensMirroredChanged)

    /**
     * Whether notification sounds should be disabled
     *
     * This does not reflect the actual mute state of the Notification Sounds
     * stream but only remembers what value was assigned to this property.
     *
     * This way you can tell whether to unmute notification sounds or not, in case
     * the user had them explicitly muted previously.
     *
     * @note This does not actually mute or unmute the actual sound stream,
     * you need to do this yourself using e.g. PulseAudio.
     */
    Q_PROPERTY(bool notificationSoundsInhibited READ notificationSoundsInhibited WRITE setNotificationSoundsInhibited NOTIFY settingsChanged)

    /**
     * Whether to update the properties immediately when they are changed on disk
     *
     * This can be undesirable for a settings dialog where outside changes
     * should not suddenly cause the UI to change.
     *
     * Default is true.
     */
    Q_PROPERTY(bool live READ live WRITE setLive NOTIFY liveChanged)

    /**
     * Whether the settings have changed and need to be saved
     *
     * @sa save()
     */
    Q_PROPERTY(bool dirty READ dirty NOTIFY dirtyChanged)

public:
    explicit Settings(QObject *parent = nullptr);
    /**
     * @deprecated
     */
    Settings(const KSharedConfig::Ptr &config, QObject *parent = nullptr);
    ~Settings() override;

    enum PopupPosition {
        CloseToWidget = 0,
        TopLeft,
        TopCenter,
        TopRight,
        BottomLeft,
        BottomCenter,
        BottomRight,
    };
    Q_ENUM(PopupPosition)

    enum NotificationBehavior {
        ShowPopups = 1 << 1,
        ShowPopupsInDoNotDisturbMode = 1 << 2,
        ShowInHistory = 1 << 3,
        ShowBadges = 1 << 4,
    };
    Q_ENUM(NotificationBehavior)
    Q_DECLARE_FLAGS(NotificationBehaviors, NotificationBehavior)
    Q_FLAG(NotificationBehaviors)

    Q_INVOKABLE NotificationBehaviors applicationBehavior(const QString &desktopEntry) const;
    Q_INVOKABLE void setApplicationBehavior(const QString &desktopEntry, NotificationBehaviors behaviors);

    Q_INVOKABLE NotificationBehaviors serviceBehavior(const QString &desktopEntry) const;
    Q_INVOKABLE void setServiceBehavior(const QString &desktopEntry, NotificationBehaviors behaviors);

    Q_INVOKABLE void registerKnownApplication(const QString &desktopEntry);
    Q_INVOKABLE void forgetKnownApplication(const QString &desktopEntry);

    Q_INVOKABLE void load();
    Q_INVOKABLE void save();
    Q_INVOKABLE void defaults();

    bool live() const;
    void setLive(bool live);

    bool dirty() const;

    bool criticalPopupsInDoNotDisturbMode() const;
    void setCriticalPopupsInDoNotDisturbMode(bool enable);

    bool keepCriticalAlwaysOnTop() const;
    void setKeepCriticalAlwaysOnTop(bool enable);

    bool lowPriorityPopups() const;
    void setLowPriorityPopups(bool enable);

    bool lowPriorityHistory() const;
    void setLowPriorityHistory(bool enable);

    PopupPosition popupPosition() const;
    void setPopupPosition(PopupPosition popupPosition);

    int popupTimeout() const;
    void setPopupTimeout(int popupTimeout);
    void resetPopupTimeout();

    bool jobsInTaskManager() const;
    void setJobsInTaskManager(bool enable);

    bool jobsInNotifications() const;
    void setJobsInNotifications(bool enable);

    bool permanentJobPopups() const;
    void setPermanentJobPopups(bool enable);

    bool badgesInTaskManager() const;
    void setBadgesInTaskManager(bool enable);

    QStringList knownApplications() const;

    QStringList popupBlacklistedApplications() const;
    QStringList popupBlacklistedServices() const;

    QStringList doNotDisturbPopupWhitelistedApplications() const;
    QStringList doNotDisturbPopupWhitelistedServices() const;

    QStringList historyBlacklistedApplications() const;
    QStringList historyBlacklistedServices() const;

    QStringList badgeBlacklistedApplications() const;

    QDateTime notificationsInhibitedUntil() const;
    void setNotificationsInhibitedUntil(const QDateTime &time);
    void resetNotificationsInhibitedUntil();

    bool notificationsInhibitedByApplication() const;
    QStringList notificationInhibitionApplications() const;
    QStringList notificationInhibitionReasons() const;

    bool inhibitNotificationsWhenScreensMirrored() const;
    void setInhibitNotificationsWhenScreensMirrored(bool mirrored);

    bool screensMirrored() const;
    void setScreensMirrored(bool enable);

    bool notificationSoundsInhibited() const;
    void setNotificationSoundsInhibited(bool inhibited);

    /**
     * Revoke application notification inhibitions.
     *
     * @note Applications are not notified of the fact that their
     * inhibition might have been taken away.
     */
    Q_INVOKABLE void revokeApplicationInhibitions();

signals:
    void settingsChanged();

    void liveChanged();
    void dirtyChanged();

    void knownApplicationsChanged();

    void notificationsInhibitedByApplicationChanged(bool notificationsInhibitedByApplication);
    void notificationInhibitionApplicationsChanged();

    void screensMirroredChanged();

private:
    class Private;
    QScopedPointer<Private> d;
};

} // namespace NotificationManager

Q_DECLARE_OPERATORS_FOR_FLAGS(NotificationManager::Settings::NotificationBehaviors)
