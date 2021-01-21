/***************************************************************************
 *   Copyright (C) 2015 Marco Martin <mart@kde.org>                        *
 *   Copyright (C) 2020 Konrad Materka <materka@gmail.com>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#ifndef DBUSSERVICEOBSERVER_H
#define DBUSSERVICEOBSERVER_H

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QRegExp>

class KPluginMetaData;
class SystemTraySettings;
class QDBusPendingCallWatcher;
class QDBusServiceWatcher;

/**
 * @brief Loading and unloading Plasmoids when DBus services come and go.
 */
class DBusServiceObserver : public QObject
{
    Q_OBJECT
public:
    explicit DBusServiceObserver(QPointer<SystemTraySettings> settings, QObject *parent = nullptr);

    void registerPlugin(const KPluginMetaData &pluginMetaData);
    void unregisterPlugin(const QString &pluginId);
    bool isDBusActivable(const QString &pluginId);

signals:
    void serviceStarted(const QString &pluginId);
    void serviceStopped(const QString &pluginId);

public Q_SLOTS:
    void initDBusActivatables();

private:
    void serviceNameFetchFinished(QDBusPendingCallWatcher *watcher);
    void serviceRegistered(const QString &service);
    void serviceUnregistered(const QString &service);

    QPointer<SystemTraySettings> m_settings;

    QDBusServiceWatcher *m_sessionServiceWatcher;
    QDBusServiceWatcher *m_systemServiceWatcher;

    QHash<QString /*plugin id*/, QRegExp /*DBus Service*/> m_dbusActivatableTasks;
    QHash<QString, int> m_dbusServiceCounts;
    bool m_dbusSessionServiceNamesFetched = false;
    bool m_dbusSystemServiceNamesFetched = false;
};

#endif // DBUSSERVICEOBSERVER_H
