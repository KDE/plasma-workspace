/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

 Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "waylandserver.h"
// ksld
#include <config-ksmserver.h>
// Wayland
#include <wayland-server.h>
#include <wayland-ksld-server-protocol.h>
// KWayland
#include <KWayland/Server/display.h>
// Qt
#include <QDBusConnection>
// system
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

namespace ScreenLocker
{

static const QString s_plasmaShellService = QStringLiteral("org.kde.plasmashell");
static const QString s_osdServicePath = QStringLiteral("/org/kde/osdService");
static const QString s_osdServiceInterface = QStringLiteral("org.kde.osdService");

WaylandServer::WaylandServer(QObject *parent)
    : QObject(parent)
{
    // connect to osd service
    QDBusConnection::sessionBus().connect(s_plasmaShellService,
                                          s_osdServicePath,
                                          s_osdServiceInterface,
                                          QStringLiteral("osdProgress"),
                                          this, SLOT(osdProgress(QString,int,QString)));
    QDBusConnection::sessionBus().connect(s_plasmaShellService,
                                          s_osdServicePath,
                                          s_osdServiceInterface,
                                          QStringLiteral("osdText"),
                                          this, SLOT(osdText(QString,QString)));
}

WaylandServer::~WaylandServer()
{
    stop();
}

int WaylandServer::start()
{
    stop();
    m_display.reset(new KWayland::Server::Display);
    m_display->start(KWayland::Server::Display::StartMode::ConnectClientsOnly);
    if (!m_display->isRunning()) {
        // failed to start the Wayland server
        return -1;
    }
    int socketPair[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, socketPair) == -1) {
        // failed creating socket
        return -1;
    }
    fcntl(socketPair[0], F_SETFD, FD_CLOEXEC);
    m_allowedClient = m_display->createClient(socketPair[0]);
    if (!m_allowedClient) {
        // failed creating the Wayland client
        stop();
        close(socketPair[0]);
        close(socketPair[1]);
        return -1;
    }
    m_interface = wl_global_create(*m_display.data(), &org_kde_ksld_interface, 2, this, bind);
    return socketPair[1];
}

void WaylandServer::stop()
{
    if (m_interface) {
        wl_global_destroy(m_interface);
        m_interface = nullptr;
    }
    m_display.reset();
    m_allowedClient = nullptr;
}

void WaylandServer::bind(wl_client *client, void *data, uint32_t version, uint32_t id)
{
    auto s = reinterpret_cast<WaylandServer*>(data);
    if (client != s->m_allowedClient->client()) {
        // a proper error would be better
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource *r = s->m_allowedClient->createResource(&org_kde_ksld_interface, qMin(version, 2u), id);
    if (!r) {
        wl_client_post_no_memory(client);
        return;
    }

    static const struct org_kde_ksld_interface s_interface = {
        x11WindowCallback
    };
    wl_resource_set_implementation(r, &s_interface, s, unbind);
    s->addResource(r);
    s->m_allowedClient->flush();
}

void WaylandServer::unbind(wl_resource *resource)
{
    reinterpret_cast<WaylandServer*>(wl_resource_get_user_data(resource))->removeResource(resource);
}

void WaylandServer::x11WindowCallback(wl_client *client, wl_resource *resource, uint32_t id)
{
    auto s = reinterpret_cast<WaylandServer*>(wl_resource_get_user_data(resource));
    if (s->m_allowedClient->client() != client) {
        return;
    }
    emit s->x11WindowAdded(id);
}

void WaylandServer::addResource(wl_resource *r)
{
    m_resources.append(r);
}

void WaylandServer::removeResource(wl_resource *r)
{
    m_resources.removeAll(r);
}

void WaylandServer::osdProgress(const QString &icon, int percent, const QString &additionalText)
{
    for (auto r : m_resources) {
        if (wl_resource_get_version(r) < 2) {
            continue;
        }
        org_kde_ksld_send_osdProgress(r, icon.toUtf8().constData(), percent, additionalText.toUtf8().constData());
        m_allowedClient->flush();
    }
}

void WaylandServer::osdText(const QString &icon, const QString &additionalText)
{
    for (auto r : m_resources) {
        if (wl_resource_get_version(r) < 2) {
            continue;
        }
        org_kde_ksld_send_osdText(r, icon.toUtf8().constData(), additionalText.toUtf8().constData());
        m_allowedClient->flush();
    }
}

}
