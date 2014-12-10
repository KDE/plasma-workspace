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
#ifndef SCREENLOCKER_WAYLANDSERVER_H
#define SCREENLOCKER_WAYLANDSERVER_H

#include <QObject>

struct wl_client;
struct wl_global;
struct wl_resource;

namespace KWayland
{
namespace Server
{
class ClientConnection;
class Display;
}
}

namespace ScreenLocker
{

class WaylandServer : public QObject
{
    Q_OBJECT
public:
    explicit WaylandServer(QObject *parent = nullptr);
    virtual ~WaylandServer();
    int start();
    void stop();

Q_SIGNALS:
    void x11WindowAdded(quint32 window);

private:
    static void bind(wl_client *client, void *data, uint32_t version, uint32_t id);
    static void unbind(wl_resource *resource);
    static void x11WindowCallback(wl_client *client, wl_resource *resource, uint32_t id);
    QScopedPointer<KWayland::Server::Display> m_display;
    KWayland::Server::ClientConnection *m_allowedClient = nullptr;
    wl_global *m_interface = nullptr;
};

}

#endif
