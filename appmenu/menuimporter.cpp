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

#include "menuimporter.h"
#include "menuimporteradaptor.h"

#include <QApplication>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusServiceWatcher>

#include <KDebug>
#include <KWindowSystem>
#include <KWindowInfo>

static const char* DBUS_SERVICE = "com.canonical.AppMenu.Registrar";
static const char* DBUS_OBJECT_PATH = "/com/canonical/AppMenu/Registrar";

// Marshalling code for DBusMenuLayoutItem
QDBusArgument &operator<<(QDBusArgument &argument, const DBusMenuLayoutItem &obj)
{
    argument.beginStructure();
    argument << obj.id << obj.properties;
    argument.beginArray(qMetaTypeId<QDBusVariant>());
    Q_FOREACH(const DBusMenuLayoutItem& child, obj.children) {
        argument << QDBusVariant(QVariant::fromValue<DBusMenuLayoutItem>(child));
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, DBusMenuLayoutItem &obj)
{
    argument.beginStructure();
    argument >> obj.id >> obj.properties;
    argument.beginArray();
    while (!argument.atEnd()) {
        QDBusVariant dbusVariant;
        argument >> dbusVariant;
        QDBusArgument childArgument = dbusVariant.variant().value<QDBusArgument>();

        DBusMenuLayoutItem child;
        childArgument >> child;
        obj.children.append(child);
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}

MenuImporter::MenuImporter(QObject* parent)
: QObject(parent)
, m_serviceWatcher(new QDBusServiceWatcher(this))
{
    qDBusRegisterMetaType<DBusMenuLayoutItem>();
    m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    m_serviceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &MenuImporter::slotServiceUnregistered);

    QDBusConnection::sessionBus().connect(QLatin1String(""), QLatin1String(""), QStringLiteral("com.canonical.dbusmenu"), QStringLiteral("LayoutUpdated"),
                                          this, SLOT(slotLayoutUpdated(uint,int)));
}

MenuImporter::~MenuImporter()
{
    QDBusConnection::sessionBus().unregisterService(DBUS_SERVICE);
    QDBusConnection::sessionBus().disconnect(QLatin1String(""), QLatin1String(""), QStringLiteral("com.canonical.dbusmenu"), QStringLiteral("LayoutUpdated"),
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

WId MenuImporter::recursiveMenuId(WId id)
{
    KWindowInfo info(id, 0, NET::WM2WindowClass);
    QString classClass = info.windowClassClass();
    WId classId = 0;

    // First look at transient windows
    WId tid = KWindowSystem::transientFor(id);
    while (tid) {
        if (serviceExist(tid)) {
            classId = tid;
            break;
        }
        tid = KWindowSystem::transientFor(tid);
    }

    if (classId == 0) {
        // Look at friends windows
        QHashIterator<WId, QString> i(m_windowClasses);
        while (i.hasNext()) {
            i.next();
            if (i.value() == classClass) {
                classId = i.key();
            }
        }
    }

    return classId;
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
    if (! m_serviceWatcher->watchedServices().contains(service)) {
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
        fakeUnityAboutToShow();
    }
}

void MenuImporter::fakeUnityAboutToShow()
{
    QDBusInterface iface(message().service(), message().path(), QStringLiteral("com.canonical.dbusmenu"));
    QDBusPendingCall call = iface.asyncCall(QStringLiteral("GetLayout"), 0, 1, QStringList());
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    watcher->setProperty("service", message().service());
    watcher->setProperty("path", message().path());
    connect(watcher, &QDBusPendingCallWatcher::finished,
        this, &MenuImporter::finishFakeUnityAboutToShow);
}

void MenuImporter::finishFakeUnityAboutToShow(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<uint, DBusMenuLayoutItem> reply = *watcher;
    if (reply.isError()) {
        kWarning() << "Call to GetLayout failed:" << reply.error().message();
        watcher->deleteLater();
        return;
    }
    QString service = watcher->property("service").toString();
    QString path = watcher->property("path").toString();
    DBusMenuLayoutItem root = reply.argumentAt<1>();

    watcher->deleteLater();

    QDBusInterface iface(service, path, QStringLiteral("com.canonical.dbusmenu"));
    Q_FOREACH(const DBusMenuLayoutItem& dbusMenuItem, root.children) {
        iface.asyncCall(QStringLiteral("AboutToShow"), dbusMenuItem.id);
    }
}
