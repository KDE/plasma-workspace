/*
 * Copyright 2012  Alex Merry <alex.merry@kdemail.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "multiplexedservice.h"

#include "multiplexer.h"
#include "playercontrol.h"

MultiplexedService::MultiplexedService(Multiplexer *multiplexer, QObject *parent)
    : Plasma::Service(parent)
{
    setObjectName(Multiplexer::sourceName + QLatin1String(" controller"));
    setName("mpris2");
    setDestination(Multiplexer::sourceName);

    connect(multiplexer, SIGNAL(activePlayerChanged(PlayerContainer*)),
            this, SLOT(activePlayerChanged(PlayerContainer*)));

    activePlayerChanged(multiplexer->activePlayer());
}

Plasma::ServiceJob* MultiplexedService::createJob(const QString& operation,
                              QMap<QString,QVariant>& parameters)
{
    if (m_control) {
        return m_control.data()->createJob(operation, parameters);
    }
    return 0;
}

void MultiplexedService::updateEnabledOperations()
{
    if (m_control) {
        foreach (const QString &op, operationNames()) {
            setOperationEnabled(op, m_control.data()->isOperationEnabled(op));
        }
    } else {
        foreach (const QString &op, operationNames()) {
            setOperationEnabled(op, false);
        }
    }
}

void MultiplexedService::activePlayerChanged(PlayerContainer *container)
{
    delete m_control.data();

    if (container) {
        m_control = new PlayerControl(container, container->getDataEngine());
        connect(m_control.data(), SIGNAL(enabledOperationsChanged()),
                this,             SLOT(updateEnabledOperations()));
    }

    updateEnabledOperations();
}

