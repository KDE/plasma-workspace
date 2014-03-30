/***************************************************************************
 *   Copyright (C) 2009 Marco Martin <notmart@gmail.com>                   *
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

#ifndef DBUSSYSTEMTRAYPROTOCOL_H
#define DBUSSYSTEMTRAYPROTOCOL_H

#include "../../protocol.h"

#include <Plasma/DataEngine>
#include <Plasma/DataEngineConsumer>

#include <QHash>

#include <QDBusConnection>


namespace SystemTray
{

class DBusSystemTrayTask;

class DBusSystemTrayProtocol : public Protocol
{
    Q_OBJECT
    friend class DBusSystemTrayTask;
public:
    DBusSystemTrayProtocol(QObject *parent);
    ~DBusSystemTrayProtocol();
    void init();

protected:
    void initRegisteredServices();

protected Q_SLOTS:
    void newTask(const QString &service);
    void cleanupTask(const QString &taskId);

private:
    void initedTask(DBusSystemTrayTask *task);

    Plasma::DataEngine *m_dataEngine;
    Plasma::DataEngineConsumer *m_dataEngineConsumer;
    QHash<QString, DBusSystemTrayTask*> m_tasks;
};

}


#endif
