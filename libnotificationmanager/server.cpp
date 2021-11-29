/*
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
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
        Q_EMIT inhibitedChanged(inhibited());
    });
    connect(d.data(), &ServerPrivate::externalInhibitedChanged, this, [this] {
        Q_EMIT inhibitedByApplicationChanged(inhibitedByApplication());
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
    Q_EMIT notificationRemoved(notificationId, reason);

    Q_EMIT d->NotificationClosed(notificationId, static_cast<uint>(reason)); // tell on DBus
}

void Server::invokeAction(uint notificationId, const QString &actionName)
{
    Q_EMIT d->ActionInvoked(notificationId, actionName);
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
