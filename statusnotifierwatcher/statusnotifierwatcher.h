/***************************************************************************
 *   Copyright 2009 by Marco Martin <notmart@gmail.com>                    *
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

#ifndef STATUSNOTIFIERWATCHER_H
#define STATUSNOTIFIERWATCHER_H

#include <kdedmodule.h>

#include <QDBusContext>
#include <QObject>
#include <QStringList>
#include <QSet>

class QDBusServiceWatcher;

class StatusNotifierWatcher : public KDEDModule, protected QDBusContext
{
    Q_OBJECT
    Q_PROPERTY(QStringList RegisteredStatusNotifierItems READ RegisteredStatusNotifierItems)
    Q_PROPERTY(bool IsStatusNotifierHostRegistered READ IsStatusNotifierHostRegistered)
    Q_PROPERTY(int ProtocolVersion READ ProtocolVersion)

public:
    StatusNotifierWatcher(QObject *parent, const QList<QVariant>&);
    ~StatusNotifierWatcher() override;

    QStringList RegisteredStatusNotifierItems() const;

    bool IsStatusNotifierHostRegistered() const;

    int ProtocolVersion() const;

public Q_SLOTS:
    void RegisterStatusNotifierItem(const QString &service);

    void RegisterStatusNotifierHost(const QString &service);

protected Q_SLOTS:
    void serviceUnregistered(const QString& name);

Q_SIGNALS:
    void StatusNotifierItemRegistered(const QString &service);
    //TODO: decide if this makes sense, the systray itself could notice the vanishing of items, but looks complete putting it here
    void StatusNotifierItemUnregistered(const QString &service);
    void StatusNotifierHostRegistered();
    void StatusNotifierHostUnregistered();

private:
    QDBusServiceWatcher *m_serviceWatcher = nullptr;
    QStringList m_registeredServices;
    QSet<QString> m_statusNotifierHostServices;
};
#endif
