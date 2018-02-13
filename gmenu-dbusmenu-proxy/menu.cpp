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
#include <QMutableListIterator>
#include <QVariantList>
#include <QWindow>

#include "dbusmenuadaptor.h"

#include "../libdbusmenuqt/dbusmenushortcut_p.h"

static const QString s_orgGtkActions = QStringLiteral("org.gtk.Actions");
static const QString s_orgGtkMenus = QStringLiteral("org.gtk.Menus");

Menu::Menu(WId winId,
           const QString &serviceName,
           const QString &applicationObjectPath,
           const QString &windowObjectPath,
           const QString &menuObjectPath)
    : QObject()
    , m_winId(winId)
    , m_serviceName(serviceName)
    , m_applicationObjectPath(applicationObjectPath)
    , m_windowObjectPath(windowObjectPath)
    , m_menuObjectPath(menuObjectPath)
{
    qDebug() << "Created menu on" << m_serviceName << "at" << m_applicationObjectPath << m_windowObjectPath << m_menuObjectPath;

    GDBusMenuTypes_register();
    DBusMenuTypes_register();

    // FIXME doesn't work work
    if (!QDBusConnection::sessionBus().connect(m_serviceName,
                                               m_menuObjectPath,
                                               s_orgGtkMenus,
                                               QStringLiteral("Changed"),
                                               this,
                                               SLOT(onMenuChanged(GMenuChangeList)))) {
        qWarning() << "Failed to subscribe to menu changes in" << m_serviceName << "at" << m_menuObjectPath;
    }

    if (!QDBusConnection::sessionBus().connect(m_serviceName,
                                               m_applicationObjectPath,
                                               s_orgGtkActions,
                                               QStringLiteral("Changed"),
                                               this,
                                               SLOT(onApplicationActionsChanged(GMenuActionsChange)))) {
        qWarning() << "Failed to subscribe to application action changes in" << m_serviceName << "at" << m_applicationObjectPath;
    }

    if (!QDBusConnection::sessionBus().connect(m_serviceName,
                                               m_windowObjectPath,
                                               s_orgGtkActions,
                                               QStringLiteral("Changed"),
                                               this,
                                               SLOT(onWindowActionsChanged(GMenuActionsChange))) ){
        qWarning() << "Failed to subscribe to window action changes in" << m_serviceName << "at" << m_windowObjectPath;
    }

    // TODO share application actions between menus of the same app?
    getActions(m_applicationObjectPath, [this](const GMenuActionMap &actions, bool ok) {
        if (ok) {
            m_applicationActions = actions;
            m_queriedApplicationActions = true;
            start();
        }
    });

    getActions(m_windowObjectPath, [this](const GMenuActionMap &actions, bool ok) {
        if (ok) {
            m_windowActions = actions;
            m_queriedWindowActions = true;
            start();
        }
    });
}

Menu::~Menu()
{
    stop(m_subscriptions);

    emit requestRemoveWindowProperties();
}

WId Menu::winId() const
{
    return m_winId;
}

QString Menu::serviceName() const
{
    return m_serviceName;
}

QString Menu::applicationObjectPath() const
{
    return m_applicationObjectPath;
}

QString Menu::windowObjectPath() const
{
    return m_windowObjectPath;
}

QString Menu::menuObjectPath() const
{
    return m_menuObjectPath;
}

QString Menu::proxyObjectPath() const
{
    return m_proxyObjectPath;
}

void Menu::start()
{
    if (!m_queriedApplicationActions || m_queriedWindowActions) {
        return;
    }

    qDebug() << "START!";
    start(0);
}


void Menu::start(uint id)
{
    qDebug() << "start" << id;

    if (m_subscriptions.contains(id)) {
        qDebug() << "Already subscribed to" << id;
        return;
    }
    // TODO watch service disappearing?

    // dbus-send --print-reply --session --dest=:1.103 /org/libreoffice/window/104857641/menus/menubar org.gtk.Menus.Start array:uint32:0

    QDBusMessage msg = QDBusMessage::createMethodCall(m_serviceName,
                                                      m_menuObjectPath,
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
            qWarning() << "Failed to start subscription to" << id << "from" << m_serviceName << "at" << m_menuObjectPath << reply.error();
        } else {
            const bool wasSubscribed = !m_subscriptions.isEmpty();

            const auto menus = reply.value();
            for (auto menu : menus) {
                m_menus[menu.id].append(menus);

                // TODO?
                emit LayoutUpdated(1 /*revision*/, treeStructureToInt(menu.id, 0, 0));

                // TODO are we subscribed to all it returns or just to the ones we requested?
                m_subscriptions.append(menu.id);
            }

            // Now resolve any aliases we might have
            // TODO Don't do this every time? ugly..
            for (auto &menu : m_menus) {
                for (auto &section : menu) {
                    QMutableListIterator<QVariantMap> it(section.items);
                    while (it.hasNext()) {
                        auto &item = it.next();

                        auto findIt = item.constFind(QStringLiteral(":section"));
                        if (findIt != item.constEnd()) {
                            // references another place, add it instead
                            GMenuSection gmenuSection = qdbus_cast<GMenuSection>(findIt->value<QDBusArgument>());

                            // TODO figure out what to do when menu changed since we'd end up shifting the indices around here
                            if (item.value(QStringLiteral("section-expanded")).toBool()) {
                                //qDebug() << "Already know section here, skip";
                                continue;
                            }

                            //qDebug() << "pls add" << gmenuSection.subscription << "at" << gmenuSection.menu << "for" << item;

                            // TODO start subscription if we don't have it
                            auto items = findSection(m_menus.value(gmenuSection.subscription), gmenuSection.menu).items;

                            // remember that this section was already expanded, we'll keep the reference around
                            // so we can show a separator line in the menu
                            item.insert(QStringLiteral("section-expanded"), true);

                            // Check whether it's an alias to an alias
                            // FIXME make generic/recursive
                            if (items.count() == 1) {
                                const auto &aliasedItem = items.constFirst();
                                auto findIt = aliasedItem.constFind(QStringLiteral(":section"));
                                if (findIt != aliasedItem.constEnd()) {
                                    GMenuSection gmenuSection2 = qdbus_cast<GMenuSection>(findIt->value<QDBusArgument>());
                                    qDebug() << "Resolved alias from" << gmenuSection.subscription << gmenuSection.menu << "to" << gmenuSection2.subscription << gmenuSection2.menu;
                                    items = findSection(m_menus.value(gmenuSection2.subscription), gmenuSection2.menu).items;
                                }
                            }

                            for (const auto &aliasedItem : qAsConst(items)) {
                                it.insert(aliasedItem);
                            }
                        }
                    }
                }
            }


            // first time we successfully requested a menu, announce that this window supports DBusMenu
            if (!m_subscriptions.isEmpty() && !wasSubscribed) {
                if (registerDBusObject()) {
                    emit requestWriteWindowProperties();
                }
            }
        }

        // When it was a delayed GetLayout request, send the reply now
        auto pendingReply = m_pendingGetLayouts.value(id);
        if (pendingReply.type() != QDBusMessage::InvalidMessage) {
            qDebug() << "replying that we now have" << id;

            auto reply = pendingReply.createReply();

            DBusMenuLayoutItem item;
            uint revision = GetLayout(treeStructureToInt(id, 0, 0), 0, {}, item);

            reply << revision << QVariant::fromValue(item);

            QDBusConnection::sessionBus().send(reply);
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
    qDebug() << "menu changed";
    for (const auto &change : changes) {
        qDebug() << change.subscription << change.menu << change.changePosition << change.itemsToRemoveCount << change.itemsToInsert;
    }
}

void Menu::onApplicationActionsChanged(const GMenuActionsChange &changes)
{
    qDebug() << "app actions changed";
    qDebug() << changes.removed << changes.enabledChanged << changes.stateChanged << changes.added.count();
}

void Menu::onWindowActionsChanged(const GMenuActionsChange &changes)
{
    qDebug() << "window actions changed";
    qDebug() << changes.removed << changes.enabledChanged << changes.stateChanged << changes.added.count();
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
            qWarning() << "Failed to get actions from" << m_serviceName << "at" << path << reply.error();
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
            qWarning() << "Failed to invoke action" << name << "on" << m_serviceName << "at" << path << reply.error();
        }
    });
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
    qDebug() << "about to show" << id << calledFromDBus();

    int subscription;
    int sectionId;
    int index;
    intToTreeStructure(id, subscription, sectionId, index);

    // This doesn't seem to happen, apps seem to just blatantly run GetLayout for unknown stuff
    if (!m_subscriptions.contains(subscription)) {
        qDebug() << "NEED TO QUERY FIRST subscription" << subscription;
        start(subscription);
        return false;
    }

    return true;
}

void Menu::Event(int id, const QString &eventId, const QDBusVariant &data, uint timestamp)
{
    qDebug() << "event" << id << eventId << data.variant() << timestamp;

    if (eventId == QLatin1String("opened")) {

    } else if (eventId == QLatin1String("closed")) {

    } else if (eventId == QLatin1String("clicked")) {
        int subscription;
        int sectionId;
        int index;

        intToTreeStructure(id, subscription, sectionId, index);

        if (index < 1) { // cannot "click" a menu
            return;
        }

        // TODO check bounds
        const auto &item = findSection(m_menus.value(subscription), sectionId).items.at(index - 1);

        const QString action = item.value(QStringLiteral("action")).toString();
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

    //qDebug() << "GIMME" << parentId << "which is" << subscription << sectionId << index;

    if (!m_subscriptions.contains(subscription)) {
        qDebug() << "not subscribed to" << subscription << ", requesting it now";

        auto oldPending = m_pendingGetLayouts.value(subscription);
        if (oldPending.type() != QDBusMessage::InvalidMessage) {
            QDBusConnection::sessionBus().send(
                oldPending.createErrorReply(
                    QStringLiteral("org.kde.plasma.gmenu_dbusmenu_proxy.Error.GetLayoutCalledRepeatedly"),
                    QStringLiteral("GetLayout got cancelled because a new similar request came in")
                )
            );
        }

        m_pendingGetLayouts.insert(subscription, message());
        setDelayedReply(true);

        start(subscription);
        return 1;
    }

    const auto sections = m_menus.value(subscription);
    if (sections.isEmpty()) {
        // TODO start?
        qWarning() << "dont have sections for" << subscription;
        return 1;
    }

    // which sections to add to the menu
    const GMenuItem &section = findSection(sections, sectionId);

    // If a particular entry is requested, see what it is and resolve as neccessary
    // for example the "File" entry on root is 0,0,1 but is a menu reference to e.g. 1,0,0
    // so resolve that and return the correct menu
    if (index > 0) {
        // non-zero index indicates item within a menu but the index in the list still starts at zero
        const auto &requestedItem = section.items.at(index - 1); // TODO bounds check

        auto it = requestedItem.constFind(QStringLiteral(":submenu"));
        if (it != requestedItem.constEnd()) {
            const GMenuSection gmenuSection = qdbus_cast<GMenuSection>(it->value<QDBusArgument>());
            return GetLayout(treeStructureToInt(gmenuSection.subscription, gmenuSection.menu, 0), recursionDepth, propertyNames, dbusItem);
        } else {
            qDebug() << "Requested a particular item" << parentId << requestedItem;
            // TODO
            return 0;
        }
    }

    dbusItem.id = parentId; // TODO
    // TODO use gMenuToDBusMenuProperties?
    dbusItem.properties = {
        {QStringLiteral("children-display"), QStringLiteral("submenu")}
    };

    auto itemsToBeAdded = section.items;

    int count = 0;

    for (const auto &item : itemsToBeAdded) {
        DBusMenuLayoutItem child{
            treeStructureToInt(section.id, sectionId, ++count),
            gMenuToDBusMenuProperties(item)
        };

        dbusItem.children.append(child);
    }

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
    QString actionName = source.value(QStringLiteral("action")).toString();
    if (actionName.isEmpty()) {
        actionName = source.value(QStringLiteral("submenu-action")).toString();
    }

    GMenuAction action;
    bool actionOk = getAction(actionName, action);
    if (actionOk) {
        enabled = action.enabled;
    }

    if (!enabled) {
        result.insert(QStringLiteral("enabled"), false);
    }

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

    if (!visible) {
        result.insert(QStringLiteral("visible"), false);
    }

    QString icon = source.value(QStringLiteral("icon")).toString();
    if (icon.isEmpty()) {
        icon = source.value(QStringLiteral("verb-icon")).toString();
    }
    if (icon.isEmpty()) {
        QString lookupName = actionName.mid(4); // FIXME
        // FIXME do properly
        static QHash<QString, QString> s_icons {
            {QStringLiteral("new-window"), QStringLiteral("window-new")},
            {QStringLiteral("new-tab"), QStringLiteral("tab-new")},
            {QStringLiteral("open"), QStringLiteral("document-open")},
            {QStringLiteral("save"), QStringLiteral("document-save")},
            {QStringLiteral("save-as"), QStringLiteral("document-save-as")},
            {QStringLiteral("save-all"), QStringLiteral("document-save-all")},
            {QStringLiteral("print"), QStringLiteral("document-print")},
            {QStringLiteral("close"), QStringLiteral("document-close")},
            {QStringLiteral("close-all"), QStringLiteral("document-close")},
            {QStringLiteral("quit"), QStringLiteral("application-exit")},

            {QStringLiteral("undo"), QStringLiteral("edit-undo")},
            {QStringLiteral("redo"), QStringLiteral("edit-redo")},
            {QStringLiteral("cut"), QStringLiteral("edit-cut")},
            {QStringLiteral("copy"), QStringLiteral("edit-copy")},
            {QStringLiteral("paste"), QStringLiteral("edit-paste")},
            {QStringLiteral("preferences"), QStringLiteral("settings-configure")},

            {QStringLiteral("fullscreen"), QStringLiteral("view-fullscreen")},

            {QStringLiteral("find"), QStringLiteral("edit-find")},

            {QStringLiteral("previous-document"), QStringLiteral("go-previous")},
            {QStringLiteral("next-document"), QStringLiteral("go-next")},

            {QStringLiteral("help"), QStringLiteral("help-contents")},
            {QStringLiteral("about"), QStringLiteral("help-about")},
            // TODO some more
        };
        icon = s_icons.value(lookupName);
    }
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

    /*const QString targetString = source.value(QStringLiteral("target")).toString();
    if (isCheckBox && !targetString.isEmpty()) {
        result.insert(QStringLiteral("toggle-type"), QStringLiteral("radio"));
    }*/

    //qDebug() << result;

    return result;
}
