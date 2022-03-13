/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <KQuickAddons/ManagedConfigModule>

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

namespace NotificationManager
{
class BehaviorSettings;
}

class KCMNotifications : public KQuickAddons::ManagedConfigModule
{
    Q_OBJECT

    Q_PROPERTY(SourcesModel *sourcesModel READ sourcesModel CONSTANT)
    Q_PROPERTY(FilterProxyModel *filteredModel READ filteredModel CONSTANT)

    Q_PROPERTY(NotificationManager::DoNotDisturbSettings *dndSettings READ dndSettings CONSTANT)
    Q_PROPERTY(NotificationManager::NotificationSettings *notificationSettings READ notificationSettings CONSTANT)
    Q_PROPERTY(NotificationManager::JobSettings *jobSettings READ jobSettings CONSTANT)
    Q_PROPERTY(NotificationManager::BadgeSettings *badgeSettings READ badgeSettings CONSTANT)
    Q_PROPERTY(bool isDefaultsBehaviorSettings READ isDefaultsBehaviorSettings NOTIFY isDefaultsBehaviorSettingsChanged)

    Q_PROPERTY(
        QKeySequence toggleDoNotDisturbShortcut READ toggleDoNotDisturbShortcut WRITE setToggleDoNotDisturbShortcut NOTIFY toggleDoNotDisturbShortcutChanged)

    // So it can show the respective settings module right away
    Q_PROPERTY(QString initialDesktopEntry READ initialDesktopEntry WRITE setInitialDesktopEntry NOTIFY initialDesktopEntryChanged)
    Q_PROPERTY(QString initialNotifyRcName READ initialNotifyRcName WRITE setInitialNotifyRcName NOTIFY initialNotifyRcNameChanged)
    Q_PROPERTY(QString initialEventId READ initialEventId WRITE setInitialEventId NOTIFY initialEventIdChanged)

public:
    KCMNotifications(QObject *parent, const KPluginMetaData &data, const QVariantList &args);
    ~KCMNotifications() override;

    SourcesModel *sourcesModel() const;
    FilterProxyModel *filteredModel() const;

    NotificationManager::DoNotDisturbSettings *dndSettings() const;
    NotificationManager::NotificationSettings *notificationSettings() const;
    NotificationManager::JobSettings *jobSettings() const;
    NotificationManager::BadgeSettings *badgeSettings() const;

    QKeySequence toggleDoNotDisturbShortcut() const;
    void setToggleDoNotDisturbShortcut(const QKeySequence &shortcut);
    Q_SIGNAL void toggleDoNotDisturbShortcutChanged();

    QString initialDesktopEntry() const;
    void setInitialDesktopEntry(const QString &desktopEntry);

    QString initialNotifyRcName() const;
    void setInitialNotifyRcName(const QString &notifyRcName);

    QString initialEventId() const;
    void setInitialEventId(const QString &eventId);

    Q_INVOKABLE void configureEvents(const QString &notifyRcName, const QString &eventId, QQuickItem *ctx = nullptr);

    Q_INVOKABLE NotificationManager::BehaviorSettings *behaviorSettings(const QModelIndex &index);

    bool isDefaultsBehaviorSettings() const;

public Q_SLOTS:
    void load() override;
    void save() override;
    void defaults() override;

Q_SIGNALS:
    void initialDesktopEntryChanged();
    void initialNotifyRcNameChanged();
    void initialEventIdChanged();
    void firstLoadDone();
    void isDefaultsBehaviorSettingsChanged();

private Q_SLOTS:
    void onDefaultsIndicatorsVisibleChanged();
    void updateModelIsDefaultStatus(const QModelIndex &index);

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
};
