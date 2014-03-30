/***************************************************************************
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>                       *
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

#ifndef PLASMOIDPROTOCOL_H
#define PLASMOIDPROTOCOL_H

#include "../../protocol.h"
#include <QHash>

class QDBusPendingCallWatcher;
class QQuickItem;
class QmlObject;

namespace Plasma {
    class Applet;
    class Containment;
    class Corona;
}

namespace SystemTray
{

class PlasmoidTask;

class PlasmoidProtocol : public Protocol
{
    Q_OBJECT
    friend class PlasmoidTask;
public:
    PlasmoidProtocol(QObject *parent);
    ~PlasmoidProtocol();

    void init();

protected Q_SLOTS:
    void newTask(const QString &service);
    void cleanupTask(const QString &taskId);

private Q_SLOTS:
    void serviceNameFetchFinished(QDBusPendingCallWatcher* watcher);
    void serviceRegistered(const QString &service);
    void serviceUnregistered(const QString &service);

private:
    void initDBusActivatables();
    void newDBusActivatableTask(const QString &pluginName, const QString &dbusService);
    QHash<QString, PlasmoidTask*> m_tasks;
    QHash<QString, int> m_knownPlugins;
    QHash<QString, QString> m_dbusActivatableTasks;
    Plasma::Containment *m_containment;
    Plasma::Applet *m_systrayApplet;
};

}


#endif
