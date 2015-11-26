/***************************************************************************
 *   Copyright (C) 2015 Marco Martin <mart@kde.org>                        *
 *
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

#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H


#include <Plasma/Containment>

class QDBusPendingCallWatcher;
class QDBusConnection;

class SystemTray : public Plasma::Containment
{
    Q_OBJECT

public:
    SystemTray( QObject *parent, const QVariantList &args );
    ~SystemTray();

    void init();

    void restorePlasmoids();
    QStringList allowedPlugins() const;
    void setAllowedPlugins(const QStringList &allowed);

    //Creates an applet *if not already existing*
    void newTask(const QString &task);

    //cleans all instances of a given applet
    void cleanupTask(const QString &task);

private Q_SLOTS:
    void serviceNameFetchFinished(QDBusPendingCallWatcher* watcher, const QDBusConnection &connection);
    void serviceRegistered(const QString &service);
    void serviceUnregistered(const QString &service);

private:
    void initDBusActivatables();
    QStringList m_allowedPlugins;
    QHash<QString, int> m_knownPlugins;
    QHash<QString, QString> m_dbusActivatableTasks;
    QHash<QString, int> m_dbusServiceCounts;
};

#endif
