/*
 * Copyright (C) 2018 Kai Uwe Broulik <kde@privat.broulik.de>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "menu.h"

#include <QDebug>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QList>
#include <QVariantList>
#include <QWindow>

#include "dbusmenuadaptor.h"

static const QString s_orgGtkMenus = QStringLiteral("org.gtk.Menus");

Menu::Menu(WId winId, const QString &serviceName, const QString &objectPath)
    : QObject()
    , m_winId(winId)
    , m_serviceName(serviceName)
    , m_objectPath(objectPath)
{
    qDebug() << "Created menu on" << m_serviceName << "at" << m_objectPath;

    // FIXME doesn't work work
    if (!QDBusConnection::sessionBus().connect(m_serviceName,
                                               m_objectPath,
                                               s_orgGtkMenus,
                                               QStringLiteral("Changed"),
                                               this,
                                               SLOT(onMenuChanged(GMenuChangeList)))) {
        qWarning() << "Failed to subscribe to menu changes in" << m_serviceName << "at" << m_objectPath;
    }

    start({0}); // root menu
}

Menu::~Menu()
{
    stop(m_subscriptions);
}

WId Menu::winId() const
{
    return m_winId;
}

QString Menu::serviceName() const
{
    return m_serviceName;
}

QString Menu::objectPath() const
{
    return m_objectPath;
}

QString Menu::proxyObjectPath() const
{
    return m_proxyObjectPath;
}

void Menu::start(const QList<uint> &ids)
{
    GDBusMenuTypes_register();

    // TODO watch service disappearing?

    // dbus-send --print-reply --session --dest=:1.103 /org/libreoffice/window/104857641/menus/menubar org.gtk.Menus.Start array:uint32:0

    QDBusMessage msg = QDBusMessage::createMethodCall(m_serviceName,
                                                      m_objectPath,
                                                      s_orgGtkMenus,
                                                      QStringLiteral("Start"));
    msg.setArguments({QVariant::fromValue(ids)});

    QDBusPendingReply<GMenuItemList> reply = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, ids](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<GMenuItemList> reply = *watcher;
        if (reply.isError()) {
            qWarning() << "Failed to start subscription to" << ids << "from" << m_serviceName << "at" << m_objectPath << reply.error();
        } else {
            const bool wasSubscribed = !m_subscriptions.isEmpty();

            const auto menus = reply.value();
            for (auto menu : menus) {
                m_menus.insert(menu.id, menu.items);
                // TODO are we subscribed to all it returns or just to the ones we requested?
                m_subscriptions.append(menu.id);
            }

            // first time we successfully requested a menu, announce that this window supports DBusMenu
            if (!m_subscriptions.isEmpty() && !wasSubscribed) {
                if (registerDBusObject()) {
                    emit requestWriteWindowProperties();
                }
            }
        }
        watcher->deleteLater();
    });
}

void Menu::stop(const QList<uint> &ids)
{
    // TODO
    Q_UNUSED(ids);
}

void Menu::onMenuChanged(const GMenuChangeList &changes)
{
    // TODO
    Q_UNUSED(changes);
}

bool Menu::registerDBusObject()
{
    Q_ASSERT(m_proxyObjectPath.isEmpty());
    Q_ASSERT(!m_menus.isEmpty());

    static int menus = 0;
    ++menus;

    const QString objectPath = QStringLiteral("/MenuBar/%1").arg(QString::number(menus));
    qDebug() << "register as object path" << objectPath;

    if (!QDBusConnection::sessionBus().registerObject(objectPath, this)) {
        qWarning() << "Failed to register object" << this << "at" << objectPath;
        return false;
    }

    new DbusmenuAdaptor(this); // do this before registering the object?

    m_proxyObjectPath = objectPath;

    return true;
}

// DBus
bool Menu::AboutToShow(int id)
{
    qDebug() << "about to show" << id;
    return false;
}

void Menu::Event(int id, const QString &eventId, const QDBusVariant &data, uint timestamp)
{
    qDebug() << "event" << id << eventId << data.variant() << timestamp;
}

DBusMenuItemList Menu::GetGroupProperties(const QList<int> &ids, const QStringList &propertyNames)
{
    qDebug() << "get group props" << ids << propertyNames;
    return DBusMenuItemList();
}

// FIXME doesn't work :(
// No such method 'GetLayout' in interface 'com.canonical.dbusmenu' at object path '/MenuBar/1' (signature 'iias')
uint Menu::GetLayout(int parentId, int recursionDepth, const QStringList &propertyNames, DBusMenuLayoutItem &item)
{
    qDebug() << "getlayout" << parentId << "depth" << recursionDepth << propertyNames;// << item.id << item.properties;

    item.id = 0;
    item.properties = {
        {QStringLiteral("title"), QStringLiteral("hello")}
    };

    return 1; // revision
}

QDBusVariant Menu::GetProperty(int id, const QString &property)
{
    qDebug() << "get property" << id << property;
    QDBusVariant value;
    return value;
}

QString Menu::status() const
{
    return QStringLiteral("normal");
}

uint Menu::version() const
{
    return 4;
}
