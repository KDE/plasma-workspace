/*
  This file is part of the KDE project.

  Copyright (c) 2011 Lionel Chauvin <megabigbug@yahoo.fr>
  Copyright (c) 2011,2012 Cédric Bellegarde <gnumdk@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
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
    QString newPath = path.isEmpty() ? DBUS_OBJECT_PATH : path;

    if (!QDBusConnection::sessionBus().registerService(m_service)) {
        return false;
    }
    new AppmenuAdaptor(this);
    QDBusConnection::sessionBus().registerObject(newPath, this);

    return true;
}

void AppmenuDBus::showMenu(int x, int y, const QString &serviceName, const QDBusObjectPath &menuObjectPath, int actionId)
{
    emit appShowMenu(x, y, serviceName, menuObjectPath, actionId);
}

void AppmenuDBus::reconfigure()
{
    emit reconfigured();
}
