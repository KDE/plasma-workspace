/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QDBusContext>
#include <QDBusObjectPath>
#include <QList>
#include <QObject>
#include <QSet>

#include "notifications.h"

class QDBusServiceWatcher;
class QTimer;

namespace NotificationManager
{
class Job;
class Settings;

class Q_DECL_HIDDEN JobsModelPrivate : public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    JobsModelPrivate(QObject *parent);
    ~JobsModelPrivate() override;

    // DBus
    // kuiserver
    void registerService(const QString &service, const QString &objectPath);
    void emitJobUrlsChanged();
    bool requiresJobTracker() const;
    QStringList registeredJobContacts() const;
    // V1
    QDBusObjectPath requestView(const QString &appName, const QString &appIconName, int capabilities);
    // V2
    QDBusObjectPath requestView(const QString &desktopEntry, int capabilities, const QVariantMap &hints);

Q_SIGNALS:
    void jobViewAboutToBeAdded(int row, NotificationManager::Job *job);
    void jobViewAdded(int row, NotificationManager::Job *job);

    void jobViewAboutToBeRemoved(int row); //, Job *job);
    void jobViewRemoved(int row);

    void jobViewChanged(int row, NotificationManager::Job *job, const QList<int> &roles);

    void serviceOwnershipLost();

    // DBus
    // kuiserver
    void jobUrlsChanged(const QStringList &urls);
    void requiresJobTrackerChanged(const bool req);

public: // stuff used by public class
    bool init();

    void remove(Job *job);
    void removeAt(int row);

    bool m_valid = false;
    QList<Job *> m_jobViews;

private:
    void unwatchJob(Job *job);
    void onServiceUnregistered(const QString &serviceName);

    void updateApplicationPercentage(const QString &desktopEntry);

    QStringList jobUrls() const;
    void scheduleUpdate(Job *job, int role);

    // Job -> serviceName
    QHash<Job *, QString> m_jobServices;
    int m_highestJobId = 1;

    QTimer *m_compressUpdatesTimer = nullptr;
    QHash<Job *, QList<int>> m_pendingDirtyRoles;

    QList<Job *> m_pendingJobViews;

    QDBusServiceWatcher *m_serviceWatcher = nullptr;
    Settings *m_settings = nullptr;
};

} // namespace NotificationManager
