/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <KQuickManagedConfigModule>

#include <QHash>
#include <QKeySequence>

#include "badgesettings.h"
#include "donotdisturbsettings.h"
#include "filterproxymodel.h"
#include "jobsettings.h"
#include "notificationsettings.h"
#include "sourcesmodel.h"

class QAction;

class NotificationsData;
class SoundThemeConfig;

namespace NotificationManager
{
class BehaviorSettings;
}

struct ca_context;

class KCMNotifications : public KQuickManagedConfigModule
{
    Q_OBJECT

    Q_PROPERTY(QString plasmaWorkspaceNotifyRcName READ plasmaWorkspaceNotifyRcName CONSTANT)

    Q_PROPERTY(SourcesModel *sourcesModel READ sourcesModel CONSTANT)
    Q_PROPERTY(FilterProxyModel *filteredModel READ filteredModel CONSTANT)

    Q_PROPERTY(NotificationManager::DoNotDisturbSettings *dndSettings READ dndSettings CONSTANT)
    Q_PROPERTY(NotificationManager::NotificationSettings *notificationSettings READ notificationSettings CONSTANT)
    Q_PROPERTY(NotificationManager::JobSettings *jobSettings READ jobSettings CONSTANT)
    Q_PROPERTY(NotificationManager::BadgeSettings *badgeSettings READ badgeSettings CONSTANT)
    Q_PROPERTY(bool isDefaultsBehaviorSettings READ isDefaultsBehaviorSettings NOTIFY isDefaultsBehaviorSettingsChanged)

    Q_PROPERTY(
        QKeySequence toggleDoNotDisturbShortcut READ toggleDoNotDisturbShortcut WRITE setToggleDoNotDisturbShortcut NOTIFY toggleDoNotDisturbShortcutChanged)

public:
    KCMNotifications(QObject *parent, const KPluginMetaData &data, const QVariantList &args);
    ~KCMNotifications() override;

    static QString plasmaWorkspaceNotifyRcName();

    SourcesModel *sourcesModel() const;
    FilterProxyModel *filteredModel() const;

    NotificationManager::DoNotDisturbSettings *dndSettings() const;
    NotificationManager::NotificationSettings *notificationSettings() const;
    NotificationManager::JobSettings *jobSettings() const;
    NotificationManager::BadgeSettings *badgeSettings() const;

    QKeySequence toggleDoNotDisturbShortcut() const;
    void setToggleDoNotDisturbShortcut(const QKeySequence &shortcut);
    Q_SIGNAL void toggleDoNotDisturbShortcutChanged();

    Q_INVOKABLE QUrl soundsLocation();
    Q_INVOKABLE void playSound(const QString &soundName);

    Q_INVOKABLE NotificationManager::BehaviorSettings *behaviorSettings(const QModelIndex &index);

    bool isDefaultsBehaviorSettings() const;

public Q_SLOTS:
    void load() override;
    void save() override;
    void defaults() override;

Q_SIGNALS:
    void navigateToComponent(const QString &desktopEntry, const QString notifyRcName, const QString eventId);
    void isDefaultsBehaviorSettingsChanged();

private Q_SLOTS:
    void onDefaultsIndicatorsVisibleChanged();
    void updateModelIsDefaultStatus(const QModelIndex &index);
    void parseArguments(const QVariantList &args);

private:
    bool isSaveNeeded() const override;
    bool isDefaults() const override;
    void createConnections(NotificationManager::BehaviorSettings *settings, const QModelIndex &index);

    SourcesModel *const m_sourcesModel;
    FilterProxyModel *const m_filteredModel;

    NotificationsData *const m_data;

    QAction *m_toggleDoNotDisturbAction;
    QKeySequence m_toggleDoNotDisturbShortcut;
    bool m_toggleDoNotDisturbShortcutDirty = false;
    bool m_firstLoad = true;

    QString m_initialDesktopEntry;
    QString m_initialNotifyRcName;
    QString m_initialEventId;

    ca_context *m_canberraContext = nullptr;
    SoundThemeConfig *m_soundThemeConfig = nullptr;
};
