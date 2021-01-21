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

#include "debug.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDebug>
#include <QVariantList>

#include <algorithm>

#include "utils.h"

static const QString s_orgGtkMenus = QStringLiteral("org.gtk.Menus");

Menu::Menu(const QString &serviceName, const QString &objectPath, QObject *parent)
    : QObject(parent)
    , m_serviceName(serviceName)
    , m_objectPath(objectPath)
{
    Q_ASSERT(!serviceName.isEmpty());
    Q_ASSERT(!m_objectPath.isEmpty());

    if (!QDBusConnection::sessionBus()
             .connect(m_serviceName, m_objectPath, s_orgGtkMenus, QStringLiteral("Changed"), this, SLOT(onMenuChanged(GMenuChangeList)))) {
        qCWarning(DBUSMENUPROXY) << "Failed to subscribe to menu changes for" << parent << "on" << serviceName << "at" << objectPath;
    }
}

Menu::~Menu() = default;

void Menu::cleanup()
{
    stop(m_subscriptions);
}

void Menu::start(uint id)
{
    if (m_subscriptions.contains(id)) {
        return;
    }

    // TODO watch service disappearing?

    // dbus-send --print-reply --session --dest=:1.103 /org/libreoffice/window/104857641/menus/menubar org.gtk.Menus.Start array:uint32:0

    QDBusMessage msg = QDBusMessage::createMethodCall(m_serviceName, m_objectPath, s_orgGtkMenus, QStringLiteral("Start"));
    msg.setArguments({QVariant::fromValue(QList<uint>{id})});

    QDBusPendingReply<GMenuItemList> reply = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, id](QDBusPendingCallWatcher *watcher) {
        QScopedPointer<QDBusPendingCallWatcher, QScopedPointerDeleteLater> watcherPtr(watcher);

        QDBusPendingReply<GMenuItemList> reply = *watcherPtr;
        if (reply.isError()) {
            qCWarning(DBUSMENUPROXY) << "Failed to start subscription to" << id << "on" << m_serviceName << "at" << m_objectPath << reply.error();
            emit failedToSubscribe(id);
        } else {
            const bool hadMenu = !m_menus.isEmpty();

            const auto menus = reply.value();
            for (const auto &menu : menus) {
                m_menus[menu.id].append(menus);
            }

            // LibreOffice on startup fails to give us some menus right away, we'll also subscribe in onMenuChanged() if necessary
            if (menus.isEmpty()) {
                qCWarning(DBUSMENUPROXY) << "Got an empty menu for" << id << "on" << m_serviceName << "at" << m_objectPath;
                return;
            }

            // TODO are we subscribed to all it returns or just to the ones we requested?
            m_subscriptions.append(id);

            // do we have a menu now? let's tell everyone
            if (!hadMenu && !m_menus.isEmpty()) {
                emit menuAppeared();
            }

            emit subscribed(id);
        }
    });
}

void Menu::stop(const QList<uint> &ids)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(m_serviceName, m_objectPath, s_orgGtkMenus, QStringLiteral("End"));
    msg.setArguments({
        QVariant::fromValue(ids) // don't let it unwrap it, hence in a variant
    });

    QDBusPendingReply<void> reply = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, ids](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<void> reply = *watcher;
        if (reply.isError()) {
            qCWarning(DBUSMENUPROXY) << "Failed to stop subscription to" << ids << "on" << m_serviceName << "at" << m_objectPath << reply.error();
        } else {
            // remove all subscriptions that we unsubscribed from
            // TODO is there a nicer algorithm for that?
            // TODO remove all m_menus also?
            m_subscriptions.erase(
                std::remove_if(m_subscriptions.begin(), m_subscriptions.end(), std::bind(&QList<uint>::contains, m_subscriptions, std::placeholders::_1)),
                m_subscriptions.end());

            if (m_subscriptions.isEmpty()) {
                emit menuDisappeared();
            }
        }
        watcher->deleteLater();
    });
}

bool Menu::hasMenu() const
{
    return !m_menus.isEmpty();
}

bool Menu::hasSubscription(uint subscription) const
{
    return m_subscriptions.contains(subscription);
}

GMenuItem Menu::getSection(int id, bool *ok) const
{
    int subscription;
    int section;
    int index;
    Utils::intToTreeStructure(id, subscription, section, index);
    return getSection(subscription, section, ok);
}

GMenuItem Menu::getSection(int subscription, int section, bool *ok) const
{
    const auto menu = m_menus.value(subscription);

    auto it = std::find_if(menu.begin(), menu.end(), [section](const GMenuItem &item) {
        return item.section == section;
    });

    if (it == menu.end()) {
        if (ok) {
            *ok = false;
        }
        return GMenuItem();
    }

    if (ok) {
        *ok = true;
    }
    return *it;
}

QVariantMap Menu::getItem(int id) const
{
    int subscription;
    int section;
    int index;
    Utils::intToTreeStructure(id, subscription, section, index);
    return getItem(subscription, section, index);
}

QVariantMap Menu::getItem(int subscription, int sectionId, int index) const
{
    bool ok;
    const GMenuItem section = getSection(subscription, sectionId, &ok);

    if (!ok) {
        return QVariantMap();
    }

    const auto items = section.items;

    if (items.count() < index) {
        qCWarning(DBUSMENUPROXY) << "Cannot get action" << subscription << sectionId << index << "which is out of bounds";
        return QVariantMap();
    }

    // 0 is the menu itself, items start at 1
    return items.at(index - 1);
}

void Menu::onMenuChanged(const GMenuChangeList &changes)
{
    const bool hadMenu = !m_menus.isEmpty();

    QVector<uint> dirtyMenus;
    QVector<uint> dirtyItems;

    for (const auto &change : changes) {
        auto updateSection = [&](GMenuItem &section) {
            // Check if the amount of inserted items is identical to the items to be removed,
            // just update the existing items and signal a change for that.
            // LibreOffice tends to do that e.g. to update its Undo menu entry
            if (change.itemsToRemoveCount == change.itemsToInsert.count()) {
                for (int i = 0; i < change.itemsToInsert.count(); ++i) {
                    const auto &newItem = change.itemsToInsert.at(i);

                    section.items[change.changePosition + i] = newItem;

                    // 0 is the menu itself, items start at 1
                    dirtyItems.append(Utils::treeStructureToInt(change.subscription, change.menu, change.changePosition + i + 1));
                }
            } else {
                for (int i = 0; i < change.itemsToRemoveCount; ++i) {
                    section.items.removeAt(change.changePosition); // TODO bounds check
                }

                for (int i = 0; i < change.itemsToInsert.count(); ++i) {
                    section.items.insert(change.changePosition + i, change.itemsToInsert.at(i));
                }

                dirtyMenus.append(Utils::treeStructureToInt(change.subscription, change.menu, 0));
            }
        };

        // shouldn't happen, it says only Start() subscribes to changes
        if (!m_subscriptions.contains(change.subscription)) {
            qCDebug(DBUSMENUPROXY) << "Got menu change for menu" << change.subscription << "that we are not subscribed to, subscribing now";
            // LibreOffice doesn't give us a menu right away but takes a while and then signals us a change
            start(change.subscription);
            continue;
        }

        auto &menu = m_menus[change.subscription];

        bool sectionFound = false;
        // TODO findSectionRef
        for (GMenuItem &section : menu) {
            if (section.section != change.menu) {
                continue;
            }

            qCInfo(DBUSMENUPROXY) << "Updating existing section" << change.menu << "in subscription" << change.subscription;

            sectionFound = true;
            updateSection(section);
            break;
        }

        // Insert new section
        if (!sectionFound) {
            qCInfo(DBUSMENUPROXY) << "Creating new section" << change.menu << "in subscription" << change.subscription;

            if (change.itemsToRemoveCount > 0) {
                qCWarning(DBUSMENUPROXY) << "Menu change requested to remove items from a new (and as such empty) section";
            }

            GMenuItem newSection;
            newSection.id = change.subscription;
            newSection.section = change.menu;
            updateSection(newSection);
            menu.append(newSection);
        }
    }

    // do we have a menu now? let's tell everyone
    if (!hadMenu && !m_menus.isEmpty()) {
        emit menuAppeared();
    } else if (hadMenu && m_menus.isEmpty()) {
        emit menuDisappeared();
    }

    if (!dirtyItems.isEmpty()) {
        emit itemsChanged(dirtyItems);
    }

    emit menusChanged(dirtyMenus);
}

void Menu::actionsChanged(const QStringList &dirtyActions, const QString &prefix)
{
    auto forEachMenuItem = [this](const std::function<bool(int subscription, int section, int index, const QVariantMap &item)> &cb) {
        for (auto it = m_menus.constBegin(), end = m_menus.constEnd(); it != end; ++it) {
            const int subscription = it.key();

            for (const auto &menu : it.value()) {
                const int section = menu.section;

                int count = 0;

                const auto items = menu.items;
                for (const auto &item : items) {
                    ++count; // 0 is a menu, entries start at 1

                    if (!cb(subscription, section, count, item)) {
                        goto loopExit; // hell yeah
                        break;
                    }
                }
            }
        }

    loopExit: // loop exit
        return;
    };

    // now find in which menus these actions are and emit a change accordingly
    QVector<uint> dirtyItems;

    for (const QString &action : dirtyActions) {
        const QString prefixedAction = prefix + action;

        forEachMenuItem([&prefixedAction, &dirtyItems](int subscription, int section, int index, const QVariantMap &item) {
            const QString actionName = Utils::itemActionName(item);

            if (actionName == prefixedAction) {
                dirtyItems.append(Utils::treeStructureToInt(subscription, section, index));
                return false; // break
            }

            return true; // continue
        });
    }

    if (!dirtyItems.isEmpty()) {
        emit itemsChanged(dirtyItems);
    }
}
