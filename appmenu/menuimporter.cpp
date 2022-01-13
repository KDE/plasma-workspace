/*
    SPDX-FileCopyrightText: 2011 Lionel Chauvin <megabigbug@yahoo.fr>
    SPDX-FileCopyrightText: 2011, 2012 Cédric Bellegarde <gnumdk@gmail.com>
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: MIT
*/

#include "menuimporter.h"
#include "dbusmenutypes_p.h"
#include "menuimporteradaptor.h"

#include <QDBusMessage>
#include <QDBusServiceWatcher>

#include <KWindowInfo>
#include <KWindowSystem>

static const char *DBUS_SERVICE = "com.canonical.AppMenu.Registrar";
static const char *DBUS_OBJECT_PATH = "/com/canonical/AppMenu/Registrar";

MenuImporter::MenuImporter(QObject *parent)
    : QObject(parent)
    , m_serviceWatcher(new QDBusServiceWatcher(this))
{
    qDBusRegisterMetaType<DBusMenuLayoutItem>();
    m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    m_serviceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &MenuImporter::slotServiceUnregistered);
}

MenuImporter::~MenuImporter()
{
    QDBusConnection::sessionBus().unregisterService(DBUS_SERVICE);
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

void MenuImporter::RegisterWindow(WId id, const QDBusObjectPath &path)
{
    KWindowInfo info(id, NET::WMWindowType, NET::WM2WindowClass);
    NET::WindowTypes mask = NET::AllTypesMask;
    auto type = info.windowType(mask);

    // Menu can try to register, right click in gimp for example
    if (type != NET::Unknown && (type & (NET::Menu | NET::DropdownMenu | NET::PopupMenu))) {
        return;
    }

    if (path.path().isEmpty()) // prevent bad dbusmenu usage
        return;

    QString service = message().service();

    QString classClass = info.windowClassClass();
    m_windowClasses.insert(id, classClass);
    m_menuServices.insert(id, service);
    m_menuPaths.insert(id, path);

    if (!m_serviceWatcher->watchedServices().contains(service)) {
        m_serviceWatcher->addWatchedService(service);
    }

    Q_EMIT WindowRegistered(id, service, path);
}

void MenuImporter::UnregisterWindow(WId id)
{
    m_menuServices.remove(id);
    m_menuPaths.remove(id);
    m_windowClasses.remove(id);

    Q_EMIT WindowUnregistered(id);
}

QString MenuImporter::GetMenuForWindow(WId id, QDBusObjectPath &path)
{
    path = m_menuPaths.value(id);
    return m_menuServices.value(id);
}

void MenuImporter::slotServiceUnregistered(const QString &service)
{
    WId id = m_menuServices.key(service);
    m_menuServices.remove(id);
    m_menuPaths.remove(id);
    m_windowClasses.remove(id);
    Q_EMIT WindowUnregistered(id);
    m_serviceWatcher->removeWatchedService(service);
}
