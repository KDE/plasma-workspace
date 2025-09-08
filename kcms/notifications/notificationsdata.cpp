/*
    SPDX-FileCopyrightText: 2020 Cyril Rossi <cyril.rossi@enioka.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "notificationsdata.h"

#include <libnotificationmanager/badgesettings.h>
#include <libnotificationmanager/behaviorsettings.h>
#include <libnotificationmanager/donotdisturbsettings.h>
#include <libnotificationmanager/jobsettings.h>
#include <libnotificationmanager/notificationsettings.h>

#include <algorithm>

using namespace Qt::StringLiterals;

NotificationsData::NotificationsData(QObject *parent)
    : KCModuleData(parent)
    , m_dndSettings(new NotificationManager::DoNotDisturbSettings(this))
    , m_notificationSettings(new NotificationManager::NotificationSettings(this))
    , m_jobSettings(new NotificationManager::JobSettings(this))
    , m_badgeSettings(new NotificationManager::BadgeSettings(this))
{
    autoRegisterSkeletons();
    readBehaviorSettings();
}

NotificationManager::DoNotDisturbSettings *NotificationsData::dndSettings() const
{
    return m_dndSettings;
}

NotificationManager::NotificationSettings *NotificationsData::notificationSettings() const
{
    return m_notificationSettings;
}

NotificationManager::JobSettings *NotificationsData::jobSettings() const
{
    return m_jobSettings;
}

NotificationManager::BadgeSettings *NotificationsData::badgeSettings() const
{
    return m_badgeSettings;
}

NotificationManager::BehaviorSettings *NotificationsData::behaviorSettings(int index) const
{
    return m_behaviorSettingsList.value(index);
}

void NotificationsData::insertBehaviorSettings(int index, NotificationManager::BehaviorSettings *settings)
{
    m_behaviorSettingsList[index] = settings;
}

void NotificationsData::loadBehaviorSettings()
{
    for (auto *behaviorSettings : std::as_const(m_behaviorSettingsList)) {
        behaviorSettings->load();
    }
}

void NotificationsData::saveBehaviorSettings()
{
    for (auto *behaviorSettings : std::as_const(m_behaviorSettingsList)) {
        behaviorSettings->save();
    }
}

void NotificationsData::defaultsBehaviorSettings()
{
    for (auto *behaviorSettings : std::as_const(m_behaviorSettingsList)) {
        behaviorSettings->setDefaults();
    }
}

bool NotificationsData::isSaveNeededBehaviorSettings() const
{
    bool needSave = std::ranges::any_of(m_behaviorSettingsList, [](const NotificationManager::BehaviorSettings *settings) {
        return settings->isSaveNeeded();
    });
    return needSave;
}

bool NotificationsData::isDefaultsBehaviorSettings() const
{
    bool notDefault = std::ranges::any_of(m_behaviorSettingsList, [](const NotificationManager::BehaviorSettings *settings) {
        return !settings->isDefaults();
    });
    return !notDefault;
}

void NotificationsData::readBehaviorSettings()
{
    KConfig config(u"plasmanotifyrc"_s, KConfig::SimpleConfig);

    for (auto groupEntry : {QStringLiteral("Applications"), QStringLiteral("Services")}) {
        KConfigGroup group(&config, groupEntry);
        for (const QString &desktopEntry : group.groupList()) {
            m_behaviorSettingsList.insert(m_behaviorSettingsList.count(), new NotificationManager::BehaviorSettings(groupEntry, desktopEntry, this));
        }
    }
}

bool NotificationsData::isDefaults() const
{
    return KCModuleData::isDefaults() && isDefaultsBehaviorSettings();
}

#include "moc_notificationsdata.cpp"
