/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2009 Marco Martin <notmart@gmail.com>                   *
 *   Copyright (C) 2009 Matthieu Gallien <matthieu_gallien@yahoo.fr>       *
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

#ifndef STATUSNOTIFIERITEM_ENGINE_H
#define STATUSNOTIFIERITEM_ENGINE_H

#include "statusnotifierwatcher_interface.h"
#include <Plasma/DataEngine>
#include <Plasma/Service>
#include <QDBusConnection>

// Define our plasma Runner
class StatusNotifierItemEngine : public Plasma::DataEngine
{
    Q_OBJECT

public:
    // Basic Create/Destroy
    StatusNotifierItemEngine(QObject *parent, const QVariantList &args);
    ~StatusNotifierItemEngine() override;
    Plasma::Service *serviceForSource(const QString &name) override;

protected:
    virtual void init();
    void newItem(const QString &service);

protected Q_SLOTS:
    void serviceChange(const QString &name, const QString &oldOwner, const QString &newOwner);
    void registerWatcher(const QString &service);
    void unregisterWatcher(const QString &service);
    void serviceRegistered(const QString &service);
    void serviceUnregistered(const QString &service);

private:
    org::kde::StatusNotifierWatcher *m_statusNotifierWatcher;
    QString m_serviceName;
    static const int s_protocolVersion = 0;
};

#endif
