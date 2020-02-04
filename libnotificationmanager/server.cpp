/*
 * Copyright 2018 Kai Uwe Broulik <kde@privat.broulik.de>
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

#include "server.h"
#include "server_p.h"

#include "notification.h"
#include "notification_p.h"

#include "debug.h"

#include <QDebug>

using namespace NotificationManager;

Server::Server(QObject *parent)
    : QObject(parent)
    , d(new ServerPrivate(this))
{
    connect(d.data(), &ServerPrivate::validChanged, this, &Server::validChanged);
    connect(d.data(), &ServerPrivate::inhibitedChanged, this, [this] {
        emit inhibitedChanged(inhibited());
    });
    connect(d.data(), &ServerPrivate::externalInhibitedChanged, this, [this] {
        emit inhibitedByApplicationChanged(inhibitedByApplication());
    });
    connect(d.data(), &ServerPrivate::externalInhibitionsChanged, this, &Server::inhibitionApplicationsChanged);
    connect(d.data(), &ServerPrivate::serviceOwnershipLost, this, &Server::serviceOwnershipLost);
}

Server::~Server() = default;

Server &Server::self()
{
    static Server s_self;
    return s_self;
}

bool Server::init()
{
    return d->init();
}

bool Server::isValid() const
{
    return d->m_valid;
}

ServerInfo *Server::currentOwner() const
{
    return d->currentOwner();
}

void Server::closeNotification(uint notificationId, CloseReason reason)
{
    emit notificationRemoved(notificationId, reason);

    emit d->NotificationClosed(notificationId, static_cast<uint>(reason)); // tell on DBus
}

void Server::invokeAction(uint notificationId, const QString &actionName)
{
    emit d->ActionInvoked(notificationId, actionName);
}

void Server::reply(const QString &dbusService, uint notificationId, const QString &text)
{
    d->sendReplyText(dbusService, notificationId, text);
}

uint Server::add(const Notification &notification)
{
    return d->add(notification);
}

bool Server::inhibited() const
{
    return d->inhibited();
}

void Server::setInhibited(bool inhibited)
{
    d->setInhibited(inhibited);
}

bool Server::inhibitedByApplication() const
{
    return d->externalInhibited();
}

QStringList Server::inhibitionApplications() const
{
    QStringList applications;
    const auto inhibitions = d->externalInhibitions();
    applications.reserve(inhibitions.count());
    for (const auto &inhibition : inhibitions) {
        applications.append(!inhibition.applicationName.isEmpty() ? inhibition.applicationName : inhibition.desktopEntry);
    }
    return applications;
}

QStringList Server::inhibitionReasons() const
{
    QStringList reasons;
    const auto inhibitions = d->externalInhibitions();
    reasons.reserve(inhibitions.count());
    for (const auto &inhibition : inhibitions) {
        reasons.append(inhibition.reason);
    }
    return reasons;
}

void Server::clearInhibitions()
{
    d->clearExternalInhibitions();
}
