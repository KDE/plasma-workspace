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

#include <QDBusContext>
#include <QDBusObjectPath>
#include <QObject>
#include <QSet>
#include <QVector>

#include "notifications.h"

class QDBusServiceWatcher;
class QTimer;

namespace NotificationManager
{
class Job;

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
    void jobViewAboutToBeAdded(int row, Job *job);
    void jobViewAdded(int row, Job *job);

    void jobViewAboutToBeRemoved(int row); //, Job *job);
    void jobViewRemoved(int row);

    void jobViewChanged(int row, Job *job, const QVector<int> &roles);

    void serviceOwnershipLost();

    // DBus
    // kuiserver
    void jobUrlsChanged(const QStringList &urls);

public: // stuff used by public class
    bool init();

    void remove(Job *job);
    void removeAt(int row);

    bool m_valid = false;
    QVector<Job *> m_jobViews;

private:
    void unwatchJob(Job *job);
    void onServiceUnregistered(const QString &serviceName);

    void updateApplicationPercentage(const QString &desktopEntry);

    QStringList jobUrls() const;
    void scheduleUpdate(Job *job, Notifications::Roles role);

    QDBusServiceWatcher *m_serviceWatcher = nullptr;
    // Job -> serviceName
    QHash<Job *, QString> m_jobServices;
    int m_highestJobId = 1;

    QTimer *m_compressUpdatesTimer = nullptr;
    QHash<Job *, QVector<int>> m_pendingDirtyRoles;

    QTimer *m_pendingJobViewsTimer = nullptr;
    QVector<Job *> m_pendingJobViews;
};

} // namespace NotificationManager
