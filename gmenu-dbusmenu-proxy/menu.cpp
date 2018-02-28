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
#include <QList>
#include <QMutableListIterator>
#include <QVariantList>

#include <KLocalizedString>

#include <algorithm>

#include "dbusmenuadaptor.h"
#include "icons.h"

#include "../libdbusmenuqt/dbusmenushortcut_p.h"

static const QString s_orgGtkActions = QStringLiteral("org.gtk.Actions");
static const QString s_orgGtkMenus = QStringLiteral("org.gtk.Menus");

Menu::Menu(const QString &serviceName)
    : QObject()
    , m_serviceName(serviceName)
{
    qCDebug(DBUSMENUPROXY) << "Created menu on" << serviceName;

    Q_ASSERT(!serviceName.isEmpty());

    GDBusMenuTypes_register();
    DBusMenuTypes_register();
}

Menu::~Menu() = default;

void Menu::init()
{
     qCDebug(DBUSMENUPROXY) << "Inited menu for" << m_winId << "on" << m_serviceName << "at app" << m_applicationObjectPath << "win" << m_windowObjectPath << "unity" << m_unityObjectPath;

     if (!QDBusConnection::sessionBus().connect(m_serviceName,
                                                m_applicationMenuObjectPath,
                                                s_orgGtkMenus,
                                                QStringLiteral("Changed"),
                                                this,
                                                SLOT(onApplicationMenuChanged(GMenuChangeList)))) {
         qCWarning(DBUSMENUPROXY) << "Failed to subscribe to application menu changes on" << m_serviceName << "at" << m_applicationMenuObjectPath;
     }

    if (!QDBusConnection::sessionBus().connect(m_serviceName,
                                               m_menuBarObjectPath,
                                               s_orgGtkMenus,
                                               QStringLiteral("Changed"),
                                               this,
                                               SLOT(onMenuBarChanged(GMenuChangeList)))) {
        qCWarning(DBUSMENUPROXY) << "Failed to subscribe to menu bar changes on" << m_serviceName << "at" << m_menuBarObjectPath;
    }

    if (!m_applicationObjectPath.isEmpty() && !QDBusConnection::sessionBus().connect(m_serviceName,
                                               m_applicationObjectPath,
                                               s_orgGtkActions,
                                               QStringLiteral("Changed"),
                                               this,
                                               SLOT(onApplicationActionsChanged(QStringList,StringBoolMap,QVariantMap,GMenuActionMap)))) {
        qCWarning(DBUSMENUPROXY) << "Failed to subscribe to application action changes on" << m_serviceName << "at" << m_applicationObjectPath;
    }

    if (!m_unityObjectPath.isEmpty() && !QDBusConnection::sessionBus().connect(m_serviceName,
                                               m_unityObjectPath,
                                               s_orgGtkActions,
                                               QStringLiteral("Changed"),
                                               this,
                                               SLOT(onUnityActionsChanged(QStringList,StringBoolMap,QVariantMap,GMenuActionMap)))) {
        qCWarning(DBUSMENUPROXY) << "Failed to subscribe to Unity action changes on" << m_serviceName << "at" << m_applicationObjectPath;
    }

    if (!m_windowObjectPath.isEmpty() && !QDBusConnection::sessionBus().connect(m_serviceName,
                                               m_windowObjectPath,
                                               s_orgGtkActions,
                                               QStringLiteral("Changed"),
                                               this,
                                               SLOT(onWindowActionsChanged(QStringList,StringBoolMap,QVariantMap,GMenuActionMap)))) {
        qCWarning(DBUSMENUPROXY) << "Failed to subscribe to window action changes on" << m_serviceName << "at" << m_windowObjectPath;
    }

    // TODO share application actions between menus of the same app?
    if (!m_applicationObjectPath.isEmpty()) {
        getActions(m_applicationObjectPath, [this](const GMenuActionMap &actions, bool ok) {
            if (ok) {
                // TODO just do all of this in getActions instead of copying it thrice
                if (m_menuInited) {
                    onApplicationActionsChanged({}, {}, {}, actions);
                } else {
                    m_applicationActions = actions;
                    initMenu();
                }
            }
        });
    }

    if (!m_unityObjectPath.isEmpty()) {
        getActions(m_unityObjectPath, [this](const GMenuActionMap &actions, bool ok) {
            if (ok) {
                if (m_menuInited) {
                    onUnityActionsChanged({}, {}, {}, actions);
                } else {
                    m_unityActions = actions;
                    initMenu();
                }
            }
        });
    }

    if (!m_windowObjectPath.isEmpty()) {
        getActions(m_windowObjectPath, [this](const GMenuActionMap &actions, bool ok) {
            if (ok) {
                if (m_menuInited) {
                    onWindowActionsChanged({}, {}, {}, actions);
                } else {
                    m_windowActions = actions;
                    initMenu();
                }
            }
        });
    }
}

void Menu::cleanup()
{
    stop(m_subscriptions);

    emit requestRemoveWindowProperties();
}

WId Menu::winId() const
{
    return m_winId;
}

void Menu::setWinId(WId winId)
{
    m_winId = winId;
}

QString Menu::serviceName() const
{
    return m_serviceName;
}

QString Menu::applicationObjectPath() const
{
    return m_applicationObjectPath;
}

void Menu::setApplicationObjectPath(const QString &applicationObjectPath)
{
    m_applicationObjectPath = applicationObjectPath;
}

QString Menu::unityObjectPath() const
{
    return m_unityObjectPath;
}

void Menu::setUnityObjectPath(const QString &unityObjectPath)
{
    m_unityObjectPath = unityObjectPath;
}

QString Menu::applicationMenuObjectPath() const
{
    return m_applicationMenuObjectPath;
}

void Menu::setApplicationMenuObjectPath(const QString &applicationMenuObjectPath)
{
    m_applicationMenuObjectPath = applicationMenuObjectPath;
}

QString Menu::menuBarObjectPath() const
{
    return m_menuBarObjectPath;
}

void Menu::setMenuBarObjectPath(const QString &menuBarObjectPath)
{
    m_menuBarObjectPath = menuBarObjectPath;
}

QString Menu::windowObjectPath() const
{
    return m_windowObjectPath;
}

void Menu::setWindowObjectPath(const QString &windowObjectPath)
{
    m_windowObjectPath = windowObjectPath;
}

QString Menu::currentMenuObjectPath() const
{
    return m_currentMenuObjectPath;
}

QString Menu::proxyObjectPath() const
{
    return m_proxyObjectPath;
}

void Menu::initMenu()
{
    if (m_menuInited) {
        return;
    }

    if (!registerDBusObject()) {
        return;
    }

    // appmenu-gtk-module always announces a menu bar on every GTK window even if there is none
    // so we subscribe to the menu bar as soon as it shows up so we can figure out
    // if we have a menu bar, an app menu, or just nothing
    start(0);

    m_menuInited = true;
}

void Menu::start(uint id)
{
    if (m_subscriptions.contains(id)) {
        return;
    }
    // TODO watch service disappearing?

    // dbus-send --print-reply --session --dest=:1.103 /org/libreoffice/window/104857641/menus/menubar org.gtk.Menus.Start array:uint32:0

    if (m_currentMenuObjectPath.isEmpty()) {
        m_currentMenuObjectPath = m_menuBarObjectPath;
    }

    if (m_currentMenuObjectPath.isEmpty()) {
        m_currentMenuObjectPath = m_applicationMenuObjectPath;
    }

    Q_ASSERT(!m_currentMenuObjectPath.isEmpty()); // we shouldn't have been created without one

    QDBusMessage msg = QDBusMessage::createMethodCall(m_serviceName,
                                                      m_currentMenuObjectPath,
                                                      s_orgGtkMenus,
                                                      QStringLiteral("Start"));
    msg.setArguments({
        QVariant::fromValue(QList<uint>{id})
    });

    QDBusPendingReply<GMenuItemList> reply = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, id](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<GMenuItemList> reply = *watcher;
        if (reply.isError()) {
            qCWarning(DBUSMENUPROXY) << "Failed to start subscription to" << id << "on" << m_serviceName << "at" << m_currentMenuObjectPath << reply.error();
        } else {
            const bool hadMenu = !m_menus.isEmpty();

            const auto menus = reply.value();
            for (auto menu : menus) {
                m_menus[menu.id].append(menus);
            }

            // LibreOffice on startup fails to give us some menus right away, we'll also subscribe in onMenuChanged() if neccessary
            if (menus.isEmpty()) {
                qCWarning(DBUSMENUPROXY) << "Got an empty menu for" << id << "on" << m_serviceName << "at" << m_currentMenuObjectPath;

                // appmenu-gtk-module always claims to have a menu bar even if it is empty
                // so when we root menu is requested but it is empty AND we have an app menu (because otherwise LibreOffice breaks)
                // then we will switch to using app menu instead
                if (id == 0 && m_currentMenuObjectPath == m_menuBarObjectPath && !m_applicationMenuObjectPath.isEmpty()) {
                    qCDebug(DBUSMENUPROXY) << "Using application menu instead";
                    m_currentMenuObjectPath = m_applicationMenuObjectPath;
                    start(id);
                }

                return;
            }

            // TODO are we subscribed to all it returns or just to the ones we requested?
            m_subscriptions.append(id);

            // do we have a menu now? let's tell everyone
            if (!hadMenu && !m_menus.isEmpty()) {
                emit requestWriteWindowProperties();
            }
        }

        // When it was a delayed GetLayout request, send the reply now
        const auto pendingReplies = m_pendingGetLayouts.values(id);
        if (!pendingReplies.isEmpty()) {
            for (const auto &pendingReply : pendingReplies) {
                if (pendingReply.type() != QDBusMessage::InvalidMessage) {
                    auto reply = pendingReply.createReply();

                    DBusMenuLayoutItem item;
                    uint revision = GetLayout(treeStructureToInt(id, 0, 0), 0, {}, item);

                    reply << revision << QVariant::fromValue(item);

                    QDBusConnection::sessionBus().send(reply);
                }
            }
            m_pendingGetLayouts.remove(id);
        } else {
            emit LayoutUpdated(2 /*revision*/, id);
        }

        watcher->deleteLater();
    });
}

void Menu::stop(const QList<uint> &ids)
{
    if (m_currentMenuObjectPath.isEmpty()) {
        qCWarning(DBUSMENUPROXY) << "Cannot stop subscriptions for" << ids << "without menu object path";
    }
    QDBusMessage msg = QDBusMessage::createMethodCall(m_serviceName,
                                                      m_currentMenuObjectPath,
                                                      s_orgGtkMenus,
                                                      QStringLiteral("End"));
    msg.setArguments({
        QVariant::fromValue(ids) // don't let it unwrap it, hence in a variant
    });

    QDBusPendingReply<void> reply = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, ids](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<void> reply = *watcher;
        if (reply.isError()) {
            qCWarning(DBUSMENUPROXY) << "Failed to stop subscription to" << ids << "on" << m_serviceName << "at" << m_currentMenuObjectPath << reply.error();
        } else {
            // remove all subscriptions that we unsubscribed from
            // TODO is there a nicer algorithm for that?
            m_subscriptions.erase(std::remove_if(m_subscriptions.begin(), m_subscriptions.end(),
                                      std::bind(&QList<uint>::contains, m_subscriptions, std::placeholders::_1)),
                                  m_subscriptions.end());
        }
    });
}

void Menu::onApplicationMenuChanged(const GMenuChangeList &changes)
{
    if (m_currentMenuObjectPath != m_applicationMenuObjectPath) {
        qCInfo(DBUSMENUPROXY) << "Application menu changed for" << m_serviceName << "on" << m_applicationMenuObjectPath << "although we're actually using" << m_currentMenuObjectPath;
        return;
    }
    menuChanged(changes);
}

void Menu::onMenuBarChanged(const GMenuChangeList &changes)
{
    if (m_currentMenuObjectPath != m_menuBarObjectPath) {
        qCInfo(DBUSMENUPROXY) << "Menu bar changed for" << m_serviceName << "on" << m_menuBarObjectPath << "although we're actually using" << m_currentMenuObjectPath;
        return;
    }
    menuChanged(changes);
}

void Menu::menuChanged(const GMenuChangeList &changes)
{
    QSet<uint> dirtyMenus;
    DBusMenuItemList dirtyItems;

    for (const auto &change : changes) {
        auto updateSection = [&](GMenuItem &section) {
            // Check if the amount of inserted items is identical to the items to be removed,
            // just update the existing items and signal a change for that.
            // LibreOffice tends to do that e.g. to update its Undo menu entry
            if (change.itemsToRemoveCount == change.itemsToInsert.count()) {
                for (int i = 0; i < change.itemsToInsert.count(); ++i) {
                    const auto &newItem = change.itemsToInsert.at(i);

                    section.items[change.changePosition + i] = newItem;

                    DBusMenuItem dBusItem{
                        // 0 is menu, items start at 1
                        treeStructureToInt(change.subscription, change.menu, change.changePosition + i + 1),
                        gMenuToDBusMenuProperties(newItem)
                    };
                    dirtyItems.append(dBusItem);
                }
            } else {
                for (int i = 0; i < change.itemsToRemoveCount; ++i) {
                    section.items.removeAt(change.changePosition); // TODO bounds check
                }

                for (int i = 0; i < change.itemsToInsert.count(); ++i) {
                    section.items.insert(change.changePosition + i, change.itemsToInsert.at(i));
                }

                dirtyMenus.insert(treeStructureToInt(change.subscription, change.menu, 0));
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

    if (!dirtyItems.isEmpty()) {
        emit ItemsPropertiesUpdated(dirtyItems, {});
    }

    for (uint menu : dirtyMenus) {
        emit LayoutUpdated(3 /*revision*/, menu);
    }
}

void Menu::onApplicationActionsChanged(const QStringList &removed, const StringBoolMap &enabledChanges, const QVariantMap &stateChanges, const GMenuActionMap &added)
{
    if (!m_menuInited) {
        return;
    }
    actionsChanged(removed, enabledChanges, stateChanges, added, m_applicationActions, QStringLiteral("app."));
}

void Menu::onUnityActionsChanged(const QStringList &removed, const StringBoolMap &enabledChanges, const QVariantMap &stateChanges, const GMenuActionMap &added)
{
    if (!m_menuInited) {
        return;
    }
    actionsChanged(removed, enabledChanges, stateChanges, added, m_unityActions, QStringLiteral("unity."));
}

void Menu::onWindowActionsChanged(const QStringList &removed, const StringBoolMap &enabledChanges, const QVariantMap &stateChanges, const GMenuActionMap &added)
{
    if (!m_menuInited) {
        return;
    }
    actionsChanged(removed, enabledChanges, stateChanges, added, m_windowActions, QStringLiteral("win."));
}

void Menu::getActions(const QString &path, const std::function<void(GMenuActionMap,bool)> &cb)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(m_serviceName,
                                                      path,
                                                      s_orgGtkActions,
                                                      QStringLiteral("DescribeAll"));

    QDBusPendingReply<GMenuActionMap> reply = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, path, cb](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<GMenuActionMap> reply = *watcher;
        if (reply.isError()) {
            qCWarning(DBUSMENUPROXY) << "Failed to get actions from" << m_serviceName << "at" << path << reply.error();
            cb({}, false);
        } else {
            cb(reply.value(), true);
        }
    });
}

bool Menu::getAction(const QString &name, GMenuAction &action) const
{
    QString lookupName;
    const GMenuActionMap *actionMap = nullptr;

    if (name.startsWith(QLatin1String("app."))) {
        lookupName = name.mid(4);
        actionMap = &m_applicationActions;
    } else if (name.startsWith(QLatin1String("unity."))) {
        lookupName = name.mid(6);
        actionMap = &m_unityActions;
    } else if (name.startsWith(QLatin1String("win."))) {
        lookupName = name.mid(4);
        actionMap = &m_windowActions;
    }

    if (!actionMap) {
        return false;
    }

    auto it = actionMap->constFind(lookupName);
    if (it == actionMap->constEnd()) {
        return false;
    }

    action = *it;
    return true;
}

void Menu::triggerAction(const QString &name, uint timestamp)
{
    QString lookupName;
    QString path;

    // TODO avoid code duplication with getAction
    if (name.startsWith(QLatin1String("app."))) {
        lookupName = name.mid(4);
        path = m_applicationObjectPath;
    } else if (name.startsWith(QLatin1String("unity."))) {
        lookupName = name.mid(6);
        path = m_unityObjectPath;
    } else if (name.startsWith(QLatin1String("win."))) {
        lookupName = name.mid(4);
        path = m_windowObjectPath;
    }

    if (path.isEmpty()) {
        return;
    }

    GMenuAction action;
    if (!getAction(name, action)) {
        return;
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(m_serviceName,
                                                      path,
                                                      s_orgGtkActions,
                                                      QStringLiteral("Activate"));
    msg << lookupName;
    // TODO use the arguments provided by "target" in the menu item
    msg << QVariant::fromValue(QVariantList());

    QVariantMap platformData;

    if (timestamp) {
        // From documentation:
        // If the startup notification id is not available, this can be just "_TIMEtime", where
        // time is the time stamp from the event triggering the call.
        // see also gtkwindow.c extract_time_from_startup_id and startup_id_is_fake
        platformData.insert(QStringLiteral("desktop-startup-id"), QStringLiteral("_TIME") + QString::number(timestamp));
    }

    msg << platformData;

    QDBusPendingReply<void> reply = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, path, name](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<void> reply = *watcher;
        if (reply.isError()) {
            qCWarning(DBUSMENUPROXY) << "Failed to invoke action" << name << "on" << m_serviceName << "at" << path << reply.error();
        }
    });
}

void Menu::actionsChanged(const QStringList &removed, const StringBoolMap &enabledChanges, const QVariantMap &stateChanges, const GMenuActionMap &added, GMenuActionMap &actions, const QString &prefix)
{
    // Collect the actions that we removed, altered, or added, so we can eventually signal changes for all menus that contain one of those actions
    QSet<QString> dirtyActions;

    // TODO I bet for most of the loops below we could use a nice short std algorithm

    for (const QString &removedAction : removed) {
        if (actions.remove(removedAction)) {
            dirtyActions.insert(removedAction);
        }
    }

    for (auto it = enabledChanges.constBegin(), end = enabledChanges.constEnd(); it != end; ++it) {
        const QString &actionName = it.key();
        const bool enabled = it.value();

        auto actionIt = actions.find(actionName);
        if (actionIt == actions.end()) {
            qCInfo(DBUSMENUPROXY) << "Got enabled changed for action" << actionName << "which we don't know";
            continue;
        }

        GMenuAction &action = *actionIt;
        if (action.enabled != enabled) {
            action.enabled = enabled;
            dirtyActions.insert(actionName);
        } else {
            qCInfo(DBUSMENUPROXY) << "Got enabled change for action" << actionName << "which didn't change it";
        }
    }

    for (auto it = stateChanges.constBegin(), end = stateChanges.constEnd(); it != end; ++it) {
        const QString &actionName = it.key();
        const QVariant &state = it.value();

        auto actionIt = actions.find(actionName);
        if (actionIt == actions.end()) {
            qCInfo(DBUSMENUPROXY) << "Got state changed for action" << actionName << "which we don't know";
            continue;
        }

        GMenuAction &action = *actionIt;

        if (action.state.isEmpty()) {
            qCDebug(DBUSMENUPROXY) << "Got new state for action" << actionName << "that didn't have any state before";
            action.state.append(state);
            dirtyActions.insert(actionName);
        } else {
            // Action state is a list but the state change only sends us a single variant, so just overwrite the first one
            QVariant &firstState = action.state.first();
            if (firstState != state) {
                firstState = state;
                dirtyActions.insert(actionName);
            } else {
                qCInfo(DBUSMENUPROXY) << "Got state change for action" << actionName << "which didn't change it";
            }
        }
    }

    // unite() will result in keys being present multiple times, do it manually and overwrite existing ones
    for (auto it = added.constBegin(), end = added.constEnd(); it != end; ++it) {
        const QString &actionName = it.key();

        if (actions.contains(actionName)) { // TODO check isInfoEnabled
            qCInfo(DBUSMENUPROXY) << "Got new action" << actionName << "that we already have, overwriting existing one";
        }

        actions.insert(actionName, it.value());

        dirtyActions.insert(actionName);
    }

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

    //qDebug() << "The following actions changed" << dirtyActions;

    // now find in which menus these actions are and emit a change accordingly
    DBusMenuItemList dirtyItems;

    for (const QString &action : dirtyActions) {
        const QString prefixedAction = prefix + action;

        forEachMenuItem([this, &prefixedAction, &dirtyItems](int subscription, int section, int index, const QVariantMap &item) {
            const QString actionName = actionNameOfItem(item);
            if (actionName == prefixedAction) {
                DBusMenuItem dBusItem{
                    treeStructureToInt(subscription, section, index),
                    gMenuToDBusMenuProperties(item)
                };
                dirtyItems.append(dBusItem);
                return false; // break
            }

            return true; // continue
        });
    }

    if (!dirtyItems.isEmpty()) {
        emit ItemsPropertiesUpdated(dirtyItems, {});
    }
}

bool Menu::registerDBusObject()
{
    Q_ASSERT(m_proxyObjectPath.isEmpty());

    static int menus = 0;
    ++menus;

    const QString objectPath = QStringLiteral("/MenuBar/%1").arg(QString::number(menus));
    qCDebug(DBUSMENUPROXY) << "Registering DBus object path" << objectPath;

    if (!QDBusConnection::sessionBus().registerObject(objectPath, this)) {
        qCWarning(DBUSMENUPROXY) << "Failed to register object";
        return false;
    }

    new DbusmenuAdaptor(this); // do this before registering the object?

    m_proxyObjectPath = objectPath;

    return true;
}

// DBus
bool Menu::AboutToShow(int id)
{
    // We always request the first time GetLayout is called and keep up-to-date internally
    // No need to have us prepare anything here
    Q_UNUSED(id);
    return false;
}

void Menu::Event(int id, const QString &eventId, const QDBusVariant &data, uint timestamp)
{
    Q_UNUSED(data);

    // GMenu dbus doesn't have any "opened" or "closed" signals, we'll only handle "clicked"

    if (eventId == QLatin1String("clicked")) {
        int subscription;
        int sectionId;
        int index;

        intToTreeStructure(id, subscription, sectionId, index);

        if (index < 1) { // cannot "click" a menu
            return;
        }

        // TODO check bounds
        const auto items = findSection(m_menus.value(subscription), sectionId).items;

        if (items.count() < index) {
            qCWarning(DBUSMENUPROXY) << "Cannot trigger action" << id << subscription << sectionId << index << "as it is out of bounds";
            return;
        }

        const QString action = items.at(index - 1).value(QStringLiteral("action")).toString();
        if (!action.isEmpty()) {
            triggerAction(action, timestamp);
        }
    }

}

DBusMenuItemList Menu::GetGroupProperties(const QList<int> &ids, const QStringList &propertyNames)
{
    Q_UNUSED(ids);
    Q_UNUSED(propertyNames);
    return DBusMenuItemList();
}

uint Menu::GetLayout(int parentId, int recursionDepth, const QStringList &propertyNames, DBusMenuLayoutItem &dbusItem)
{
    Q_UNUSED(recursionDepth); // TODO
    Q_UNUSED(propertyNames);

    int subscription;
    int sectionId;
    int index;

    intToTreeStructure(parentId, subscription, sectionId, index);

    if (!m_subscriptions.contains(subscription)) {
        // let's serve multiple similar requests in one go once we've processed them
        m_pendingGetLayouts.insertMulti(subscription, message());
        setDelayedReply(true);

        start(subscription);
        return 1;
    }

    const auto sections = m_menus.value(subscription);
    if (sections.isEmpty()) {
        qCDebug(DBUSMENUPROXY) << "There are no sections for requested subscription" << subscription << "with" << parentId;
        return 1;
    }

    // which sections to add to the menu
    const GMenuItem &section = findSection(sections, sectionId);

    // If a particular entry is requested, see what it is and resolve as neccessary
    // for example the "File" entry on root is 0,0,1 but is a menu reference to e.g. 1,0,0
    // so resolve that and return the correct menu
    if (index > 0) {
        // non-zero index indicates item within a menu but the index in the list still starts at zero
        if (section.items.count() < index) {
            qCDebug(DBUSMENUPROXY) << "Requested index" << index << "on" << subscription << "at" << sectionId << "with" << parentId << "is out of bounds";
            return 0;
        }

        const auto &requestedItem = section.items.at(index - 1);

        auto it = requestedItem.constFind(QStringLiteral(":submenu"));
        if (it != requestedItem.constEnd()) {
            const GMenuSection gmenuSection = qdbus_cast<GMenuSection>(it->value<QDBusArgument>());
            return GetLayout(treeStructureToInt(gmenuSection.subscription, gmenuSection.menu, 0), recursionDepth, propertyNames, dbusItem);
        } else {
            // TODO
            return 0;
        }
    }

    dbusItem.id = parentId; // TODO
    dbusItem.properties = {
        {QStringLiteral("label"), i18n("Menu")}, // TODO use application name?
        {QStringLiteral("children-display"), QStringLiteral("submenu")}
    };

    int count = 0;

    const auto itemsToBeAdded = section.items;
    for (const auto &item : itemsToBeAdded) {

        DBusMenuLayoutItem child{
            treeStructureToInt(section.id, sectionId, ++count),
            gMenuToDBusMenuProperties(item),
            {} // children
        };
        dbusItem.children.append(child);

        // Now resolve section aliases
        auto it = item.constFind(QStringLiteral(":section"));
        if (it != item.constEnd()) {

            // references another place, add it instead
            GMenuSection gmenuSection = qdbus_cast<GMenuSection>(it->value<QDBusArgument>());

            // remember where the item came from and give it an appropriate ID
            // so updates signalled by the app will map to the right place
            int originalSubscription = gmenuSection.subscription;
            int originalMenu = gmenuSection.menu;

            // TODO start subscription if we don't have it
            auto items = findSection(m_menus.value(gmenuSection.subscription), gmenuSection.menu).items;

            // Check whether it's an alias to an alias
            // FIXME make generic/recursive
            if (items.count() == 1) {
                const auto &aliasedItem = items.constFirst();
                auto findIt = aliasedItem.constFind(QStringLiteral(":section"));
                if (findIt != aliasedItem.constEnd()) {
                    GMenuSection gmenuSection2 = qdbus_cast<GMenuSection>(findIt->value<QDBusArgument>());
                    items = findSection(m_menus.value(gmenuSection2.subscription), gmenuSection2.menu).items;

                    originalSubscription = gmenuSection2.subscription;
                    originalMenu = gmenuSection2.menu;
                }
            }

            int aliasedCount = 0;
            for (const auto &aliasedItem : qAsConst(items)) {
                DBusMenuLayoutItem aliasedChild{
                    treeStructureToInt(originalSubscription, originalMenu, ++aliasedCount),
                    gMenuToDBusMenuProperties(aliasedItem),
                    {} // children
                };
                dbusItem.children.append(aliasedChild);
            }
        }
    }

    // revision, unused in libdbusmenuqt
    return 1;
}

QDBusVariant Menu::GetProperty(int id, const QString &property)
{
    Q_UNUSED(id);
    Q_UNUSED(property);
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

int Menu::treeStructureToInt(int subscription, int section, int index)
{
    return subscription * 1000000 + section * 1000 + index;
}

void Menu::intToTreeStructure(int source, int &subscription, int &section, int &index)
{
    // TODO some better math :) or bit shifting or something
    index = source % 1000;
    section = (source / 1000) % 1000;
    subscription = source / 1000000;
}

GMenuItem Menu::findSection(const QList<GMenuItem> &list, int section)
{
    // TODO algorithm?
    for (const GMenuItem &item : list) {
        if (item.section == section) {
            return item;
        }
    }
    return GMenuItem();
}

QString Menu::actionNameOfItem(const QVariantMap &item)
{
    QString actionName = item.value(QStringLiteral("action")).toString();
    if (actionName.isEmpty()) {
        actionName = item.value(QStringLiteral("submenu-action")).toString();
    }
    return actionName;
}

QVariantMap Menu::gMenuToDBusMenuProperties(const QVariantMap &source) const
{
    QVariantMap result;

    result.insert(QStringLiteral("label"), source.value(QStringLiteral("label")).toString());

    if (source.contains(QStringLiteral(":section"))) {
        result.insert(QStringLiteral("type"), QStringLiteral("separator"));
    }

    const bool isMenu = source.contains(QStringLiteral(":submenu"));
    if (isMenu) {
        result.insert(QStringLiteral("children-display"), QStringLiteral("submenu"));
    }

    QString accel = source.value(QStringLiteral("accel")).toString();
    if (!accel.isEmpty()) {
        QStringList shortcut;

        // TODO use regexp or something
        if (accel.contains(QLatin1String("<Primary>")) || accel.contains(QLatin1String("<Control>"))) {
            shortcut.append(QStringLiteral("Control"));
            accel.remove(QLatin1String("<Primary>"));
            accel.remove(QLatin1String("<Control>"));
        }

        if (accel.contains(QLatin1String("<Shift>"))) {
            shortcut.append(QStringLiteral("Shift"));
            accel.remove(QLatin1String("<Shift>"));
        }

        if (accel.contains(QLatin1String("<Alt>"))) {
            shortcut.append(QStringLiteral("Alt"));
            accel.remove(QLatin1String("<Alt>"));
        }

        if (accel.contains(QLatin1String("<Super>"))) {
            shortcut.append(QStringLiteral("Super"));
            accel.remove(QLatin1String("<Super>"));
        }

        if (!accel.isEmpty()) {
            // TODO replace "+" by "plus" and "-" by "minus"
            shortcut.append(accel);

            // TODO does gmenu support multiple?
            DBusMenuShortcut dbusShortcut;
            dbusShortcut.append(shortcut); // don't let it unwrap the list we append

            result.insert(QStringLiteral("shortcut"), QVariant::fromValue(dbusShortcut));
        }
    }

    bool enabled = true;
    const QString actionName = actionNameOfItem(source);

    GMenuAction action;
    // if no action is specified this is fine but if there is an action we don't have
    // disable the menu entry
    bool actionOk = true;
    if (!actionName.isEmpty()) {
        actionOk = getAction(actionName, action);
        enabled = actionOk && action.enabled;
    }

    // we used to only send this if not enabled but then dbusmenuimporter does not
    // update the enabled state when it changes from disabled to enabled
    result.insert(QStringLiteral("enabled"), enabled);

    bool visible = true;
    const QString hiddenWhen = source.value(QStringLiteral("hidden-when")).toString();
    if (hiddenWhen == QLatin1String("action-disabled") && (!actionOk || !enabled)) {
        visible = false;
    } else if (hiddenWhen == QLatin1String("action-missing") && !actionOk) {
        visible = false;
    // While we have Global Menu we don't have macOS menu (where Quit, Help, etc is separate)
    } else if (hiddenWhen == QLatin1String("macos-menubar")) {
        visible = true;
    }

    result.insert(QStringLiteral("visible"), visible);

    QString icon = source.value(QStringLiteral("icon")).toString();
    if (icon.isEmpty()) {
        icon = source.value(QStringLiteral("verb-icon")).toString();
    }

    icon = Icons::actionIcon(actionName);
    if (!icon.isEmpty()) {
        result.insert(QStringLiteral("icon-name"), icon);
    }

    if (actionOk) {
        const auto args = action.state;
        if (args.count() == 1) {
            const auto &firstArg = args.first();
            // assume this is a checkbox
            if (firstArg.canConvert<bool>() && !isMenu) {
                result.insert(QStringLiteral("toggle-type"), QStringLiteral("checkbox"));
                if (firstArg.toBool()) {
                    result.insert(QStringLiteral("toggle-state"), 1);
                }
            }
        }
    }

    return result;
}
