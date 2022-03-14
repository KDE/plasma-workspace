/*
    SPDX-FileCopyrightText: 2011 Lionel Chauvin <megabigbug@yahoo.fr>
    SPDX-FileCopyrightText: 2011, 2012 Cédric Bellegarde <gnumdk@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "appmenu_dbus.h"
#include "appmenuadaptor.h"
#include "kdbusimporter.h"

#include <QApplication>
#include <QDBusMessage>
#include <QDBusServiceWatcher>

static const char *DBUS_SERVICE = "org.kde.kappmenu";
static const char *DBUS_OBJECT_PATH = "/KAppMenu";

AppmenuDBus::AppmenuDBus(QObject *parent)
    : QObject(parent)
{
}

AppmenuDBus::~AppmenuDBus()
{
}

bool AppmenuDBus::connectToBus(const QString &service, const QString &path)
{
    m_service = service.isEmpty() ? DBUS_SERVICE : service;
    const QString newPath = path.isEmpty() ? DBUS_OBJECT_PATH : path;

    if (!QDBusConnection::sessionBus().registerService(m_service)) {
        return false;
    }
    new AppmenuAdaptor(this);
    QDBusConnection::sessionBus().registerObject(newPath, this);

    return true;
}

void AppmenuDBus::showMenu(int x, int y, const QString &serviceName, const QDBusObjectPath &menuObjectPath, int actionId)
{
    Q_EMIT appShowMenu(x, y, serviceName, menuObjectPath, actionId);
}

void AppmenuDBus::reconfigure()
{
    Q_EMIT reconfigured();
}
