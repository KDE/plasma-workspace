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

#include "settings.h"

#include <QDebug>

#include <KConfigWatcher>
#include <KService>

#include "debug.h"
#include "mirroredscreenstracker_p.h"
#include "server.h"

// Settings
#include "badgesettings.h"
#include "donotdisturbsettings.h"
#include "jobsettings.h"
#include "notificationsettings.h"

namespace NotificationManager
{
constexpr const char s_configFile[] = "plasmanotifyrc";
}

using namespace NotificationManager;

class Q_DECL_HIDDEN Settings::Private
{
public:
    explicit Private(Settings *q);
    ~Private();

    void setDirty(bool dirty);

    Settings::NotificationBehaviors groupBehavior(const KConfigGroup &group) const;
    void setGroupBehavior(KConfigGroup &group, const Settings::NotificationBehaviors &behavior);

    KConfigGroup servicesGroup() const;
    KConfigGroup applicationsGroup() const;

    QStringList behaviorMatchesList(const KConfigGroup &group, Settings::NotificationBehavior behavior, bool on) const;

    Settings *q;

    KSharedConfig::Ptr config;

    KConfigWatcher::Ptr watcher;
    QMetaObject::Connection watcherConnection;

    MirroredScreensTracker::Ptr mirroredScreensTracker;

    DoNotDisturbSettings dndSettings;
    NotificationSettings notificationSettings;
    JobSettings jobSettings;
    BadgeSettings badgeSettings;

    bool live = false; // set to true initially in constructor
    bool dirty = false;
};

Settings::Private::Private(Settings *q)
    : q(q)
{
}

Settings::Private::~Private() = default;

void Settings::Private::setDirty(bool dirty)
{
    if (this->dirty != dirty) {
        this->dirty = dirty;
        emit q->dirtyChanged();
    }
}

Settings::NotificationBehaviors Settings::Private::groupBehavior(const KConfigGroup &group) const
{
    Settings::NotificationBehaviors behaviors;
    behaviors.setFlag(Settings::ShowPopups, group.readEntry("ShowPopups", true));
    // show popups in dnd mode implies the show popups
    behaviors.setFlag(Settings::ShowPopupsInDoNotDisturbMode, behaviors.testFlag(Settings::ShowPopups) && group.readEntry("ShowPopupsInDndMode", false));
    behaviors.setFlag(Settings::ShowInHistory, group.readEntry("ShowInHistory", true));
    behaviors.setFlag(Settings::ShowBadges, group.readEntry("ShowBadges", true));
    return behaviors;
}

void Settings::Private::setGroupBehavior(KConfigGroup &group, const Settings::NotificationBehaviors &behavior)
{
    if (groupBehavior(group) == behavior) {
        return;
    }

    const bool showPopups = behavior.testFlag(Settings::ShowPopups);
    if (showPopups && !group.hasDefault("ShowPopups")) {
        group.revertToDefault("ShowPopups", KConfigBase::Notify);
    } else {
        group.writeEntry("ShowPopups", showPopups, KConfigBase::Notify);
    }

    const bool showPopupsInDndMode = behavior.testFlag(Settings::ShowPopupsInDoNotDisturbMode);
    if (!showPopupsInDndMode && !group.hasDefault("ShowPopupsInDndMode")) {
        group.revertToDefault("ShowPopupsInDndMode", KConfigBase::Notify);
    } else {
        group.writeEntry("ShowPopupsInDndMode", showPopupsInDndMode, KConfigBase::Notify);
    }

    const bool showInHistory = behavior.testFlag(Settings::ShowInHistory);
    if (showInHistory && !group.hasDefault("ShowInHistory")) {
        group.revertToDefault("ShowInHistory", KConfig::Notify);
    } else {
        group.writeEntry("ShowInHistory", showInHistory, KConfigBase::Notify);
    }

    const bool showBadges = behavior.testFlag(Settings::ShowBadges);
    if (showBadges && !group.hasDefault("ShowBadges")) {
        group.revertToDefault("ShowBadges", KConfigBase::Notify);
    } else {
        group.writeEntry("ShowBadges", showBadges, KConfigBase::Notify);
    }

    setDirty(true);
}

KConfigGroup Settings::Private::servicesGroup() const
{
    return config->group("Services");
}

KConfigGroup Settings::Private::applicationsGroup() const
{
    return config->group("Applications");
}

QStringList Settings::Private::behaviorMatchesList(const KConfigGroup &group, Settings::NotificationBehavior behavior, bool on) const
{
    QStringList matches;

    const QStringList apps = group.groupList();
    for (const QString &app : apps) {
        if (groupBehavior(group.group(app)).testFlag(behavior) == on) {
            matches.append(app);
        }
    }

    return matches;
}

Settings::Settings(QObject *parent)
    : QObject(parent)
    , d(new Private(this))
{
    d->config = KSharedConfig::openConfig(s_configFile);

    setLive(true);

    connect(&Server::self(), &Server::inhibitedByApplicationChanged, this, &Settings::notificationsInhibitedByApplicationChanged);
    connect(&Server::self(), &Server::inhibitionApplicationsChanged, this, &Settings::notificationInhibitionApplicationsChanged);

    if (d->dndSettings.whenScreensMirrored()) {
        d->mirroredScreensTracker = MirroredScreensTracker::createTracker();
        connect(d->mirroredScreensTracker.data(), &MirroredScreensTracker::screensMirroredChanged, this, &Settings::screensMirroredChanged);
    }
}

Settings::Settings(const KSharedConfig::Ptr &config, QObject *parent)
    : Settings(parent)
{
    d->config = config;
}

Settings::~Settings()
{
    d->config->markAsClean();
}

Settings::NotificationBehaviors Settings::applicationBehavior(const QString &desktopEntry) const
{
    return d->groupBehavior(d->applicationsGroup().group(desktopEntry));
}

void Settings::setApplicationBehavior(const QString &desktopEntry, NotificationBehaviors behaviors)
{
    KConfigGroup group(d->applicationsGroup().group(desktopEntry));
    d->setGroupBehavior(group, behaviors);
}

Settings::NotificationBehaviors Settings::serviceBehavior(const QString &notifyRcName) const
{
    return d->groupBehavior(d->servicesGroup().group(notifyRcName));
}

void Settings::setServiceBehavior(const QString &notifyRcName, NotificationBehaviors behaviors)
{
    KConfigGroup group(d->servicesGroup().group(notifyRcName));
    d->setGroupBehavior(group, behaviors);
}

void Settings::registerKnownApplication(const QString &desktopEntry)
{
    KService::Ptr service = KService::serviceByDesktopName(desktopEntry);
    if (!service) {
        qCDebug(NOTIFICATIONMANAGER) << "Application" << desktopEntry << "cannot be registered as seen application since there is no service for it";
        return;
    }

    if (service->noDisplay()) {
        qCDebug(NOTIFICATIONMANAGER) << "Application" << desktopEntry << "will not be registered as seen application since it's marked as NoDisplay";
        return;
    }

    if (knownApplications().contains(desktopEntry)) {
        return;
    }

    d->applicationsGroup().group(desktopEntry).writeEntry("Seen", true);

    emit knownApplicationsChanged();
}

void Settings::forgetKnownApplication(const QString &desktopEntry)
{
    if (!knownApplications().contains(desktopEntry)) {
        return;
    }

    // Only remove applications that were added through registerKnownApplication
    if (!d->applicationsGroup().group(desktopEntry).readEntry("Seen", false)) {
        qCDebug(NOTIFICATIONMANAGER) << "Application" << desktopEntry << "will not be removed from seen applications since it wasn't one.";
        return;
    }

    d->applicationsGroup().deleteGroup(desktopEntry);

    emit knownApplicationsChanged();
}

void Settings::load()
{
    d->config->markAsClean();
    d->config->reparseConfiguration();
    d->dndSettings.load();
    d->notificationSettings.load();
    d->jobSettings.load();
    d->badgeSettings.load();
    emit settingsChanged();
    d->setDirty(false);
}

void Settings::save()
{
    d->dndSettings.save();
    d->notificationSettings.save();
    d->jobSettings.save();
    d->badgeSettings.save();

    d->config->sync();
    d->setDirty(false);
}

void Settings::defaults()
{
    d->dndSettings.setDefaults();
    d->notificationSettings.setDefaults();
    d->jobSettings.setDefaults();
    d->badgeSettings.setDefaults();
    emit settingsChanged();
    d->setDirty(false);
}

bool Settings::live() const
{
    return d->live;
}

void Settings::setLive(bool live)
{
    if (live == d->live) {
        return;
    }

    d->live = live;

    if (live) {
        d->watcher = KConfigWatcher::create(d->config);
        d->watcherConnection = connect(d->watcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group, const QByteArrayList &names) {
            Q_UNUSED(names);

            if (group.name() == QLatin1String("DoNotDisturb")) {
                d->dndSettings.load();

                bool emitScreensMirroredChanged = false;
                if (d->dndSettings.whenScreensMirrored()) {
                    if (!d->mirroredScreensTracker) {
                        d->mirroredScreensTracker = MirroredScreensTracker::createTracker();
                        emitScreensMirroredChanged = d->mirroredScreensTracker->screensMirrored();
                        connect(d->mirroredScreensTracker.data(), &MirroredScreensTracker::screensMirroredChanged, this, &Settings::screensMirroredChanged);
                    }
                } else if (d->mirroredScreensTracker) {
                    emitScreensMirroredChanged = d->mirroredScreensTracker->screensMirrored();
                    d->mirroredScreensTracker.reset();
                }

                if (emitScreensMirroredChanged) {
                    emit screensMirroredChanged();
                }
            } else if (group.name() == QLatin1String("Notifications")) {
                d->notificationSettings.load();
            } else if (group.name() == QLatin1String("Jobs")) {
                d->jobSettings.load();
            } else if (group.name() == QLatin1String("Badges")) {
                d->badgeSettings.load();
            }

            emit settingsChanged();
        });
    } else {
        disconnect(d->watcherConnection);
        d->watcherConnection = QMetaObject::Connection();
        d->watcher.reset();
    }

    emit liveChanged();
}

bool Settings::dirty() const
{
    // KConfigSkeleton doesn't write into the KConfig until calling save()
    // so we need to track d->config->isDirty() manually
    return d->dirty;
}

bool Settings::keepCriticalAlwaysOnTop() const
{
    return d->notificationSettings.criticalAlwaysOnTop();
}

void Settings::setKeepCriticalAlwaysOnTop(bool enable)
{
    if (this->keepCriticalAlwaysOnTop() == enable) {
        return;
    }
    d->notificationSettings.setCriticalAlwaysOnTop(enable);
    d->setDirty(true);
}

bool Settings::criticalPopupsInDoNotDisturbMode() const
{
    return d->notificationSettings.criticalInDndMode();
}

void Settings::setCriticalPopupsInDoNotDisturbMode(bool enable)
{
    if (this->criticalPopupsInDoNotDisturbMode() == enable) {
        return;
    }
    d->notificationSettings.setCriticalInDndMode(enable);
    d->setDirty(true);
}

bool Settings::lowPriorityPopups() const
{
    return d->notificationSettings.lowPriorityPopups();
}

void Settings::setLowPriorityPopups(bool enable)
{
    if (this->lowPriorityPopups() == enable) {
        return;
    }
    d->notificationSettings.setLowPriorityPopups(enable);
    d->setDirty(true);
}

bool Settings::lowPriorityHistory() const
{
    return d->notificationSettings.lowPriorityHistory();
}

void Settings::setLowPriorityHistory(bool enable)
{
    if (this->lowPriorityHistory() == enable) {
        return;
    }
    d->notificationSettings.setLowPriorityHistory(enable);
    d->setDirty(true);
}

Settings::PopupPosition Settings::popupPosition() const
{
    return static_cast<Settings::PopupPosition>(d->notificationSettings.popupPosition());
}

void Settings::setPopupPosition(Settings::PopupPosition position)
{
    if (this->popupPosition() == position) {
        return;
    }
    d->notificationSettings.setPopupPosition(position);
    d->setDirty(true);
}

int Settings::popupTimeout() const
{
    return d->notificationSettings.popupTimeout();
}

void Settings::setPopupTimeout(int timeout)
{
    if (this->popupTimeout() == timeout) {
        return;
    }
    d->notificationSettings.setPopupTimeout(timeout);
    d->setDirty(true);
}

void Settings::resetPopupTimeout()
{
    setPopupTimeout(d->notificationSettings.defaultPopupTimeoutValue());
}

bool Settings::jobsInTaskManager() const
{
    return d->jobSettings.inTaskManager();
}

void Settings::setJobsInTaskManager(bool enable)
{
    if (jobsInTaskManager() == enable) {
        return;
    }
    d->jobSettings.setInTaskManager(enable);
    d->setDirty(true);
}

bool Settings::jobsInNotifications() const
{
    return d->jobSettings.inNotifications();
}
void Settings::setJobsInNotifications(bool enable)
{
    if (jobsInNotifications() == enable) {
        return;
    }
    d->jobSettings.setInNotifications(enable);
    d->setDirty(true);
}

bool Settings::permanentJobPopups() const
{
    return d->jobSettings.permanentPopups();
}

void Settings::setPermanentJobPopups(bool enable)
{
    if (permanentJobPopups() == enable) {
        return;
    }
    d->jobSettings.setPermanentPopups(enable);
    d->setDirty(true);
}

bool Settings::badgesInTaskManager() const
{
    return d->badgeSettings.inTaskManager();
}

void Settings::setBadgesInTaskManager(bool enable)
{
    if (badgesInTaskManager() == enable) {
        return;
    }
    d->badgeSettings.setInTaskManager(enable);
    d->setDirty(true);
}

QStringList Settings::knownApplications() const
{
    return d->applicationsGroup().groupList();
}

QStringList Settings::popupBlacklistedApplications() const
{
    return d->behaviorMatchesList(d->applicationsGroup(), ShowPopups, false);
}

QStringList Settings::popupBlacklistedServices() const
{
    return d->behaviorMatchesList(d->servicesGroup(), ShowPopups, false);
}

QStringList Settings::doNotDisturbPopupWhitelistedApplications() const
{
    return d->behaviorMatchesList(d->applicationsGroup(), ShowPopupsInDoNotDisturbMode, true);
}

QStringList Settings::doNotDisturbPopupWhitelistedServices() const
{
    return d->behaviorMatchesList(d->servicesGroup(), ShowPopupsInDoNotDisturbMode, true);
}

QStringList Settings::historyBlacklistedApplications() const
{
    return d->behaviorMatchesList(d->applicationsGroup(), ShowInHistory, false);
}

QStringList Settings::historyBlacklistedServices() const
{
    return d->behaviorMatchesList(d->servicesGroup(), ShowInHistory, false);
}

QStringList Settings::badgeBlacklistedApplications() const
{
    return d->behaviorMatchesList(d->applicationsGroup(), ShowBadges, false);
}

QDateTime Settings::notificationsInhibitedUntil() const
{
    return d->dndSettings.until();
}

void Settings::setNotificationsInhibitedUntil(const QDateTime &time)
{
    d->dndSettings.setUntil(time);
    d->setDirty(true);
}

void Settings::resetNotificationsInhibitedUntil()
{
    setNotificationsInhibitedUntil(QDateTime()); // FIXME d->dndSettings.defaultUntilValue());
}

bool Settings::notificationsInhibitedByApplication() const
{
    return Server::self().inhibitedByApplication();
}

QStringList Settings::notificationInhibitionApplications() const
{
    return Server::self().inhibitionApplications();
}

QStringList Settings::notificationInhibitionReasons() const
{
    return Server::self().inhibitionReasons();
}

bool Settings::inhibitNotificationsWhenScreensMirrored() const
{
    return d->dndSettings.whenScreensMirrored();
}

void Settings::setInhibitNotificationsWhenScreensMirrored(bool inhibit)
{
    if (inhibit == inhibitNotificationsWhenScreensMirrored()) {
        return;
    }

    d->dndSettings.setWhenScreensMirrored(inhibit);
    d->setDirty(true);
}

bool Settings::screensMirrored() const
{
    return d->mirroredScreensTracker && d->mirroredScreensTracker->screensMirrored();
}

void Settings::setScreensMirrored(bool mirrored)
{
    if (mirrored) {
        qCWarning(NOTIFICATIONMANAGER) << "Cannot forcefully set screens mirrored";
        return;
    }

    if (d->mirroredScreensTracker) {
        d->mirroredScreensTracker->setScreensMirrored(mirrored);
    }
}

void Settings::revokeApplicationInhibitions()
{
    Server::self().clearInhibitions();
}

bool Settings::notificationSoundsInhibited() const
{
    return d->dndSettings.notificationSoundsMuted();
}

void Settings::setNotificationSoundsInhibited(bool inhibited)
{
    if (inhibited == notificationSoundsInhibited()) {
        return;
    }

    d->dndSettings.setNotificationSoundsMuted(inhibited);
    d->setDirty(true);
}
