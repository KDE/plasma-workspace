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

#include "notificationserver.h"
#include "debug.h"

// Settings
#include "donotdisturbsettings.h"
#include "notificationsettings.h"
#include "jobsettings.h"
#include "badgesettings.h"

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

    QStringList blacklist(const KConfigGroup &group, Settings::NotificationBehavior behavior) const;

    Settings *q;

    KSharedConfig::Ptr config;

    KConfigWatcher::Ptr watcher;
    QMetaObject::Connection watcherConnection;

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
    behaviors.setFlag(Settings::ShowPopupsInDoNotDisturbMode, group.readEntry("ShowPopupsInDndMode", false));
    behaviors.setFlag(Settings::ShowInHistory, group.readEntry("ShowInHistory", true));
    behaviors.setFlag(Settings::ShowBadges, group.readEntry("ShowBadges", true));
    return behaviors;
}

void Settings::Private::setGroupBehavior(KConfigGroup &group, const Settings::NotificationBehaviors &behavior)
{
    if (groupBehavior(group) == behavior) {
        return;
    }

    if (behavior.testFlag(Settings::ShowPopups)) {
        group.revertToDefault("ShowPopups", KConfigBase::Notify);
    } else {
        group.writeEntry("ShowPopups", false, KConfigBase::Notify);
    }

    if (behavior.testFlag(Settings::ShowPopupsInDoNotDisturbMode)) {
        group.writeEntry("ShowPopupsInDndMode", true, KConfigBase::Notify);
    } else {
        group.revertToDefault("ShowPopupsInDndMode", KConfigBase::Notify);
    }

    if (behavior.testFlag(Settings::ShowInHistory)) {
        group.revertToDefault("ShowInHistory", KConfig::Notify);
    } else {
        group.writeEntry("ShowInHistory", false, KConfigBase::Notify);
    }

    if (behavior.testFlag(Settings::ShowBadges)) {
        group.revertToDefault("ShowBadges", KConfigBase::Notify);
    } else {
        group.writeEntry("ShowBadges", false, KConfigBase::Notify);
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

QStringList Settings::Private::blacklist(const KConfigGroup &group, Settings::NotificationBehavior behavior) const
{
    QStringList blacklist;

    const QStringList apps = group.groupList();
    for (const QString &app : apps) {
        if (!groupBehavior(group.group(app)).testFlag(behavior)) {
            blacklist.append(app);
        }
    }

    return blacklist;
}

Settings::Settings(QObject *parent)
    // FIXME static thing for config file name
    : Settings(KSharedConfig::openConfig(QStringLiteral("plasmanotifyrc")), parent)
{

}

Settings::Settings(const KSharedConfig::Ptr &config, QObject *parent)
    : QObject(parent)
    , d(new Private(this))
{
    d->config = config;

    DoNotDisturbSettings::instance(config);
    connect(DoNotDisturbSettings::self(), &DoNotDisturbSettings::configChanged, this, [] {
        qDebug() << "dnd cfg changed";
    });

    NotificationSettings::instance(config);
    connect(NotificationSettings::self(), &NotificationSettings::configChanged, this, [] {
        qDebug() << "notification cfg changed";
    });

    JobSettings::instance(config);
    connect(JobSettings::self(), &JobSettings::configChanged, this, [] {
        qDebug() << "job cfg changed";
    });

    BadgeSettings::instance(config);
    connect(BadgeSettings::self(), &BadgeSettings::configChanged, this, [] {
        qDebug() << "badge cfg changed";
    });

    setLive(true);

    connect(&NotificationServer::self(), &NotificationServer::inhibitedChanged, this, &Settings::notificationsInhibitedChanged);
}

Settings::~Settings() = default;

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
    //d->config->sync();

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

    d->applicationsGroup().deleteGroup(desktopEntry); // notify?
    //d->config->sync();

    emit knownApplicationsChanged();
}

void Settings::load()
{
    DoNotDisturbSettings::self()->load();
    NotificationSettings::self()->load();
    JobSettings::self()->load();
    BadgeSettings::self()->load();
    emit settingsChanged();
    d->setDirty(false);
}

void Settings::save()
{
    DoNotDisturbSettings::self()->save();
    NotificationSettings::self()->save();
    JobSettings::self()->save();
    BadgeSettings::self()->save();

    d->config->sync();
    d->setDirty(false);
}

void Settings::defaults()
{
    DoNotDisturbSettings::self()->setDefaults();
    NotificationSettings::self()->setDefaults();
    JobSettings::self()->setDefaults();
    BadgeSettings::self()->setDefaults();
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
        d->watcherConnection = connect(d->watcher.data(), &KConfigWatcher::configChanged, this,
            [this](const KConfigGroup &group, const QByteArrayList &names) {
                Q_UNUSED(names);

                if (group.name() == QLatin1String("DoNotDisturb")) {
                    DoNotDisturbSettings::self()->load();
                } else if (group.name() == QLatin1String("Notifications")) {
                    NotificationSettings::self()->load();
                } else if (group.name() == QLatin1String("Jobs")) {
                    JobSettings::self()->load();
                } else if (group.name() == QLatin1String("Badges")) {
                    BadgeSettings::self()->load();
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
    return NotificationSettings::criticalAlwaysOnTop();
}

void Settings::setKeepCriticalAlwaysOnTop(bool enable)
{
    if (this->keepCriticalAlwaysOnTop() == enable) {
        return;
    }
    NotificationSettings::setCriticalAlwaysOnTop(enable);
    d->setDirty(true);
}

bool Settings::criticalPopupsInDoNotDisturbMode() const
{
    return NotificationSettings::criticalInDndMode();
}

void Settings::setCriticalPopupsInDoNotDisturbMode(bool enable)
{
    if (this->criticalPopupsInDoNotDisturbMode() == enable) {
        return;
    }
    NotificationSettings::setCriticalInDndMode(enable);
    d->setDirty(true);
}

bool Settings::lowPriorityPopups() const
{
    return NotificationSettings::lowPriorityPopups();
}

void Settings::setLowPriorityPopups(bool enable)
{
    if (this->lowPriorityPopups() == enable) {
        return;
    }
    NotificationSettings::setLowPriorityPopups(enable);
    d->setDirty(true);
}

Settings::PopupPosition Settings::popupPosition() const
{
    return NotificationSettings::popupPosition();
}

void Settings::setPopupPosition(Settings::PopupPosition position)
{
    if (this->popupPosition() == position) {
        return;
    }
    NotificationSettings::setPopupPosition(position);
    d->setDirty(true);
}

int Settings::popupTimeout() const
{
    return NotificationSettings::popupTimeout();
}

void Settings::setPopupTimeout(int timeout)
{
    if (this->popupTimeout() == timeout) {
        return;
    }
    NotificationSettings::setPopupTimeout(timeout);
    d->setDirty(true);
}

void Settings::resetPopupTimeout()
{
    setPopupTimeout(NotificationSettings::defaultPopupTimeoutValue());
}

bool Settings::jobsInTaskManager() const
{
    return JobSettings::inTaskManager();
}

void Settings::setJobsInTaskManager(bool enable)
{
    if (jobsInTaskManager() == enable) {
        return;
    }
    JobSettings::setInTaskManager(enable);
    d->setDirty(true);
}

bool Settings::jobsInNotifications() const
{
    return JobSettings::inNotifications();
}
void Settings::setJobsInNotifications(bool enable)
{
    if (jobsInNotifications() == enable) {
        return;
    }
    JobSettings::setInNotifications(enable);
    d->setDirty(true);
}

bool Settings::permanentJobPopups() const
{
    return JobSettings::permanentPopups();
}

void Settings::setPermanentJobPopups(bool enable)
{
    if (permanentJobPopups() == enable) {
        return;
    }
    JobSettings::setPermanentPopups(enable);
    d->setDirty(true);
}

bool Settings::badgesInTaskManager() const
{
    return BadgeSettings::inTaskManager();
}

void Settings::setBadgesInTaskManager(bool enable)
{
    if (badgesInTaskManager() == enable) {
        return;
    }
    BadgeSettings::setInTaskManager(enable);
    d->setDirty(true);
}

QStringList Settings::knownApplications() const
{
    return d->applicationsGroup().groupList();
}

QStringList Settings::popupBlacklistedApplications() const
{
    return d->blacklist(d->applicationsGroup(), ShowPopups);
}

QStringList Settings::popupBlacklistedServices() const
{
    return d->blacklist(d->servicesGroup(), ShowPopups);
}

QStringList Settings::historyBlacklistedApplications() const
{
    return d->blacklist(d->applicationsGroup(), ShowInHistory);
}

QStringList Settings::historyBlacklistedServices() const
{
    return d->blacklist(d->servicesGroup(), ShowInHistory);
}

QDateTime Settings::notificationsInhibitedUntil() const
{
    return DoNotDisturbSettings::until();
}

void Settings::setNotificationsInhibitedUntil(const QDateTime &time)
{
    DoNotDisturbSettings::setUntil(time);
    d->setDirty(true);
}

void Settings::resetNotificationsInhibitedUntil()
{
    setNotificationsInhibitedUntil(QDateTime());// DoNotDisturbSettings::defaultUntilValue());
}

bool Settings::notificationsInhibited() const
{
    return NotificationServer::self().inhibited();

}
