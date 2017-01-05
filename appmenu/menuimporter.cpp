/*
  This file is part of the KDE project.

  Copyright (c) 2011 Lionel Chauvin <megabigbug@yahoo.fr>
  Copyright (c) 2011,2012 Cédric Bellegarde <gnumdk@gmail.com>
  Copyright (c) 2016 Kai Uwe Broulik <kde@privat.broulik.de>

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

#include "menuimporter.h"
#include "menuimporteradaptor.h"
#include "dbusmenutypes_p.h"

#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusServiceWatcher>

#include <KWindowSystem>
#include <KWindowInfo>

static const char* DBUS_SERVICE = "com.canonical.AppMenu.Registrar";
static const char* DBUS_OBJECT_PATH = "/com/canonical/AppMenu/Registrar";

MenuImporter::MenuImporter(QObject* parent)
: QObject(parent)
, m_serviceWatcher(new QDBusServiceWatcher(this))
{
    qDBusRegisterMetaType<DBusMenuLayoutItem>();
    m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    m_serviceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &MenuImporter::slotServiceUnregistered);

    QDBusConnection::sessionBus().connect(QString(), QString(), QStringLiteral("com.canonical.dbusmenu"), QStringLiteral("LayoutUpdated"),
                                          this, SLOT(slotLayoutUpdated(uint,int)));
}

MenuImporter::~MenuImporter()
{
    QDBusConnection::sessionBus().unregisterService(DBUS_SERVICE);
    QDBusConnection::sessionBus().disconnect(QString(), QString(), QStringLiteral("com.canonical.dbusmenu"), QStringLiteral("LayoutUpdated"),
                                             this, SLOT(slotLayoutUpdated(uint,int)));
}

bool MenuImporter::connectToBus()
{
    if (!QDBusConnection::sessionBus().registerService(DBUS_SERVICE)) {
        return false;
    }
    new MenuImporterAdaptor(this);
    QDBusConnection::sessionBus().registerObject(DBUS_OBJECT_PATH, this);

    return true;
}

void MenuImporter::RegisterWindow(WId id, const QDBusObjectPath& path)
{
    KWindowInfo info(id, NET::WMWindowType, NET::WM2WindowClass);
    NET::WindowTypes mask = NET::AllTypesMask;

    // Menu can try to register, right click in gimp for exemple
    if (info.windowType(mask) & (NET::Menu|NET::DropdownMenu|NET::PopupMenu)) {
        return;
    }

    if (path.path().isEmpty()) //prevent bad dbusmenu usage
        return;

    QString service = message().service();

    QString classClass = info.windowClassClass();
    m_windowClasses.insert(id, classClass);
    m_menuServices.insert(id, service);
    m_menuPaths.insert(id, path);

    if (!m_serviceWatcher->watchedServices().contains(service)) {
        m_serviceWatcher->addWatchedService(service);
    }

    emit WindowRegistered(id, service, path);
}

void MenuImporter::UnregisterWindow(WId id)
{
    m_menuServices.remove(id);
    m_menuPaths.remove(id);
    m_windowClasses.remove(id);

    emit WindowUnregistered(id);
}

QString MenuImporter::GetMenuForWindow(WId id, QDBusObjectPath& path)
{
    path = m_menuPaths.value(id);
    return m_menuServices.value(id);
}

void MenuImporter::slotServiceUnregistered(const QString& service)
{
    WId id = m_menuServices.key(service);
    m_menuServices.remove(id);
    m_menuPaths.remove(id);
    m_windowClasses.remove(id);
    emit WindowUnregistered(id);
    m_serviceWatcher->removeWatchedService(service);
}

void MenuImporter::slotLayoutUpdated(uint /*revision*/, int parentId)
{
    // Fake unity-panel-service weird behavior of calling aboutToShow on
    // startup. This is necessary for Firefox menubar to work correctly at
    // startup.
    // See: https://bugs.launchpad.net/plasma-idget-menubar/+bug/878165

    if (parentId == 0) { //root menu
        fakeUnityAboutToShow(message().service(), QDBusObjectPath(message().path()));
    }
}

void MenuImporter::fakeUnityAboutToShow(const QString &service, const QDBusObjectPath &menuObjectPath)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(service, menuObjectPath.path(),
                                                      QStringLiteral("com.canonical.dbusmenu"),
                                                      QStringLiteral("GetLayout"));
    msg.setArguments({0, 1, QStringList()});

    QDBusPendingReply<uint, DBusMenuLayoutItem> reply = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, [=](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<uint, DBusMenuLayoutItem> reply = *watcher;
        if (reply.isError()) {
            qWarning() << "Call to GetLayout failed:" << reply.error().message();
        } else {
            const DBusMenuLayoutItem &root = reply.argumentAt<1>();

            for (const auto &item : root.children) {
                QDBusMessage msg = QDBusMessage::createMethodCall(service, menuObjectPath.path(), QStringLiteral("com.canonical.dbusmenu"), QStringLiteral("AboutToShow"));
                msg.setArguments({item.id});
                QDBusConnection::sessionBus().asyncCall(msg);
            }
        }
        watcher->deleteLater();
    });
}
