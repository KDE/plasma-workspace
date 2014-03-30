/***************************************************************************
 *   dbussystemtrayprotocol.cpp                                            *
 *                                                                         *
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

#include "dbussystemtraytask.h"
#include "dbussystemtrayprotocol.h"
#include "debug.h"


namespace SystemTray
{

DBusSystemTrayProtocol::DBusSystemTrayProtocol(QObject *parent)
    : Protocol(parent),
      m_dataEngine(0),
      m_dataEngineConsumer(new Plasma::DataEngineConsumer),
      m_tasks()
{
    m_dataEngine = m_dataEngineConsumer->dataEngine("statusnotifieritem");
}

DBusSystemTrayProtocol::~DBusSystemTrayProtocol()
{
    delete m_dataEngineConsumer;
}

void DBusSystemTrayProtocol::init()
{
    qCDebug(SYSTEMTRAY) << "ST Dataengine" << m_dataEngine->isValid();
    if (m_dataEngine->isValid()) {
        initRegisteredServices();
        connect(m_dataEngine, SIGNAL(sourceAdded(QString)),
                this, SLOT(newTask(QString)));
        connect(m_dataEngine, SIGNAL(sourceRemoved(QString)),
                this, SLOT(cleanupTask(QString)));
    }
}

void DBusSystemTrayProtocol::newTask(const QString &service)
{
    qCDebug(SYSTEMTRAY) << "ST new task " << service;
    if (m_tasks.contains(service)) {
        return;
    }

    DBusSystemTrayTask *task = new DBusSystemTrayTask(service, m_dataEngine, this);

    m_tasks[service] = task;
}

void DBusSystemTrayProtocol::cleanupTask(const QString &service)
{
    DBusSystemTrayTask *task = m_tasks.value(service);

    if (task) {
        m_dataEngine->disconnectSource(service, task);
        m_tasks.remove(service);
        if (task->isValid()) {
            emit task->destroyed(task);
        }
        task->deleteLater();
    }
}

void DBusSystemTrayProtocol::initedTask(DBusSystemTrayTask *task)
{
    emit taskCreated(task);
}

void DBusSystemTrayProtocol::initRegisteredServices()
{
    if (m_dataEngine->isValid()) {
        QStringList registeredItems = m_dataEngine->sources();
        foreach (const QString &service, registeredItems) {
            newTask(service);
        }
    }
}

}

#include "dbussystemtrayprotocol.moc"
