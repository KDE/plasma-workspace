/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "dbusmenumodel.h"
#include "dbusmenu_interface.h"
#include "dbusmenumodel_p.h"
#include "dbusmenushortcut_p.h"
#include "dbusmenutypes_p.h"
#include "debug.h"
#include "utils_p.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QPixmap>

static QIcon iconByName(const QString &name)
{
    return QIcon::fromTheme(name);
}

static QIcon iconFromData(const QByteArray &data)
{
    QPixmap pixmap;
    if (pixmap.loadFromData(data)) {
        return QIcon(pixmap);
    }

    return QIcon();
}

static QKeySequence shortcutFromString(const QVariant &value)
{
    QDBusArgument arg = value.value<QDBusArgument>();
    DBusMenuShortcut dmShortcut;
    arg >> dmShortcut;
    return dmShortcut.toKeySequence();
}

DBusMenuModelItem::DBusMenuModelItem(int id, const QVariantMap &data, QObject *parent)
    : QObject(parent)
    , m_id(id)
{
    for (const auto &[key, value] : data.asKeyValueRange()) {
        if (key == QLatin1String("label")) {
            m_label = swapMnemonicChar(value.toString(), QLatin1Char('_'), QLatin1Char('&'));
        } else if (key == QLatin1String("enabled")) {
            m_enabled = value.toBool();
        } else if (key == QLatin1String("visible")) {
            m_visible = value.toBool();
        } else if (key == QLatin1String("toggle-state")) {
            m_checked = value.toInt();
        } else if (key == QLatin1String("icon-name")) {
            m_icon = iconByName(value.toString());
        } else if (key == QLatin1String("icon-data")) {
            const QByteArray data = value.toByteArray();
            m_icon = iconFromData(data);
            m_iconHash = qHash(data);
        } else if (key == QLatin1String("shortcut")) {
            m_shortcut = shortcutFromString(value);
        } else if (key == QLatin1String("toggle-type")) {
            m_toggleType = value.toString();
        } else if (key == QLatin1String("type")) {
            m_separator = value == QLatin1String("separator");
        } else if (key == QLatin1String("children-display")) {
            m_submenu = value == QLatin1String("submenu");
        } else {
            qCWarning(DBUSMENUQT) << "unknown key:" << key;
        }
    }
}

DBusMenuModelItem::~DBusMenuModelItem()
{
    qDeleteAll(m_childItems);
    m_childItems.clear();
}

QList<int> DBusMenuModelItem::update(const QVariantMap &data)
{
    QList<int> diff;

    for (const auto &[key, value] : data.asKeyValueRange()) {
        if (key == QLatin1String("label")) {
            const QString label = swapMnemonicChar(value.toString(), QLatin1Char('_'), QLatin1Char('&'));
            if (m_label != label) {
                m_label = label;
                diff << Qt::DisplayRole;
            }
        } else if (key == QLatin1String("enabled")) {
            const bool enabled = value.toBool();
            if (m_enabled != enabled) {
                m_enabled = enabled;
                diff << DBusMenuModel::EnabledRole;
            }
        } else if (key == QLatin1String("visible")) {
            const bool visible = value.toBool();
            if (m_visible != visible) {
                m_visible = visible;
                diff << DBusMenuModel::VisibleRole;
            }
        } else if (key == QLatin1String("toggle-state")) {
            const bool checked = value.toInt();
            if (m_checked != checked) {
                m_checked = checked;
                diff << DBusMenuModel::CheckedRole;
            }
        } else if (key == QLatin1String("icon-name")) {
            const QString iconName = value.toString();
            if (m_icon.name() != iconName) {
                m_icon = iconByName(iconName);
                m_iconHash = std::nullopt;
                diff << Qt::DecorationRole;
            }
        } else if (key == QLatin1String("icon-data")) {
            const QByteArray data = value.toByteArray();
            const size_t iconHash = qHash(data);
            if (m_iconHash != iconHash) {
                m_icon = iconFromData(data);
                m_iconHash = iconHash;
                diff << Qt::DecorationRole;
            }
        } else if (key == QLatin1String("shortcut")) {
            const QKeySequence shortcut = shortcutFromString(value);
            if (m_shortcut != shortcut) {
                m_shortcut = shortcut;
                diff << DBusMenuModel::ShortcutRole;
            }
        } else if (key == QLatin1String("toggle-type")) {
            // Intentionally left blank.
        } else if (key == QLatin1String("type")) {
            // Intentionally left blank.
        } else if (key == QLatin1String("children-display")) {
            // Intentionally left blank.
        } else {
            qCWarning(DBUSMENUQT) << "unknown key:" << key;
        }
    }

    return diff;
}

QList<int> DBusMenuModelItem::reset(const QStringList &keys)
{
    QList<int> diff;

    for (const auto &key : keys) {
        if (key == QLatin1String("label")) {
            if (!m_label.isEmpty()) {
                m_label = QString();
                diff << Qt::DisplayRole;
            }
        } else if (key == QLatin1String("enabled")) {
            if (!m_enabled) {
                m_enabled = true;
                diff << DBusMenuModel::EnabledRole;
            }
        } else if (key == QLatin1String("visible")) {
            if (!m_visible) {
                m_visible = true;
                diff << DBusMenuModel::VisibleRole;
            }
        } else if (key == QLatin1String("toggle-state")) {
            // Intentionally left blanl.
        } else if (key == QLatin1String("icon-name") || key == QLatin1String("icon-data")) {
            if (!m_icon.isNull()) {
                m_icon = QIcon();
                m_iconHash = std::nullopt;
                diff << Qt::DecorationRole;
            }
        } else if (key == QLatin1String("shortcut")) {
            if (!m_shortcut.isEmpty()) {
                m_shortcut = QKeySequence();
                diff << DBusMenuModel::ShortcutRole;
            }
        } else if (key == QLatin1String("toggle-type")) {
            // Intentionally left blank.
        } else if (key == QLatin1String("type")) {
            // Intentionally left blank.
        } else if (key == QLatin1String("children-display")) {
            // Intentionally left blank.
        } else {
            qCWarning(DBUSMENUQT) << "unknown key:" << key;
        }
    }

    return diff;
}

int DBusMenuModelItem::id() const
{
    return m_id;
}

QString DBusMenuModelItem::label() const
{
    return m_label;
}

QIcon DBusMenuModelItem::icon() const
{
    return m_icon;
}

bool DBusMenuModelItem::isEnabled() const
{
    return m_enabled;
}

bool DBusMenuModelItem::isVisible() const
{
    return m_visible;
}

bool DBusMenuModelItem::isChecked() const
{
    return m_checked;
}

bool DBusMenuModelItem::isSubmenu() const
{
    return m_submenu || !m_childItems.isEmpty();
}

bool DBusMenuModelItem::isSeparator() const
{
    return m_separator;
}

QString DBusMenuModelItem::toggleType() const
{
    return m_toggleType;
}

QKeySequence DBusMenuModelItem::shortcut() const
{
    return m_shortcut;
}

DBusMenuModelItem *DBusMenuModelItem::parentItem() const
{
    return m_parentItem;
}

QList<DBusMenuModelItem *> DBusMenuModelItem::childItems() const
{
    return m_childItems;
}

int DBusMenuModelItem::childCount() const
{
    return m_childItems.size();
}

void DBusMenuModelItem::insertChild(DBusMenuModelItem *item, int index)
{
    m_childItems.insert(index, item);
    item->m_parentItem = this;
}

void DBusMenuModelItem::removeChild(DBusMenuModelItem *item)
{
    m_childItems.removeOne(item);
    item->m_parentItem = nullptr;
}

void DBusMenuModelItem::moveChild(int sourceIndex, int targetIndex)
{
    m_childItems.move(sourceIndex, targetIndex);
}

DBusMenuModel::DBusMenuModel(const QString &serviceName, const QString &objectPath, QObject *parent)
    : QAbstractItemModel(parent)
    , d(std::make_unique<DBusMenuModelPrivate>())
{
    DBusMenuTypes_register();
    d->interface = std::make_unique<DBusMenuInterface>(serviceName, objectPath, QDBusConnection::sessionBus());

    connect(d->interface.get(), &DBusMenuInterface::ItemActivationRequested, this, &DBusMenuModel::onItemActivationRequested);
    connect(d->interface.get(), &DBusMenuInterface::ItemsPropertiesUpdated, this, &DBusMenuModel::onItemsPropertiesUpdated);
    connect(d->interface.get(), &DBusMenuInterface::LayoutUpdated, this, &DBusMenuModel::onLayoutUpdated);

    d->dirtyTimer.setInterval(0);
    d->dirtyTimer.setSingleShot(true);
    connect(&d->dirtyTimer, &QTimer::timeout, this, [this]() {
        for (const int &id : d->dirtyItems) {
            fetchLayout(id);
        }
        d->dirtyItems.clear();
    });

    d->rootItem.reset(createItem(0));
}

DBusMenuModel::~DBusMenuModel()
{
    d->rootItem.reset();
}

int DBusMenuModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

int DBusMenuModel::rowCount(const QModelIndex &parent) const
{
    if (const auto item = itemByIndex(parent)) {
        return item->childCount();
    } else {
        return 0;
    }
}

QModelIndex DBusMenuModel::index(DBusMenuModelItem *item) const
{
    if (!item) {
        return QModelIndex();
    }

    if (d->rootItem.get() == item) {
        return QModelIndex();
    } else {
        const DBusMenuModelItem *parentItem = item->parentItem();
        const int row = parentItem->childItems().indexOf(item);
        return createIndex(row, 0, item);
    }
}

QModelIndex DBusMenuModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || column != 0) {
        return QModelIndex();
    }

    DBusMenuModelItem *item = itemByIndex(parent);
    if (item && row < item->childCount()) {
        return createIndex(row, column, item->childItems().at(row));
    }

    return QModelIndex();
}

QModelIndex DBusMenuModel::parent(const QModelIndex &child) const
{
    if (auto item = itemByIndex(child)) {
        return index(item->parentItem());
    } else {
        return QModelIndex();
    }
}

bool DBusMenuModel::hasChildren(const QModelIndex &parent) const
{
    if (auto item = itemByIndex(parent)) {
        return item->isSubmenu();
    } else {
        return false;
    }
}

QHash<int, QByteArray> DBusMenuModel::roleNames() const
{
    auto roleNames = QAbstractItemModel::roleNames();
    roleNames.insert(EnabledRole, QByteArrayLiteral("enabled"));
    roleNames.insert(VisibleRole, QByteArrayLiteral("visible"));
    roleNames.insert(CheckedRole, QByteArrayLiteral("checked"));
    roleNames.insert(SubmenuRole, QByteArrayLiteral("submenu"));
    roleNames.insert(SeparatorRole, QByteArrayLiteral("separator"));
    roleNames.insert(ToggleTypeRole, QByteArrayLiteral("toggleType"));
    roleNames.insert(ShortcutRole, QByteArrayLiteral("shortcut"));
    return roleNames;
}

QVariant DBusMenuModel::data(const QModelIndex &index, int role) const
{
    const DBusMenuModelItem *item = itemByIndex(index);
    if (!item) {
        return QVariant();
    }

    switch (role) {
    case Qt::DecorationRole:
        return item->icon();

    case Qt::DisplayRole:
        return item->label();

    case EnabledRole:
        return item->isEnabled();

    case VisibleRole:
        return item->isVisible();

    case CheckedRole:
        return item->isChecked();

    case SubmenuRole:
        return item->isSubmenu();

    case SeparatorRole:
        return item->isSeparator();

    case ToggleTypeRole:
        return item->toggleType();

    case ShortcutRole:
        return item->shortcut();
    }

    return QVariant();
}

bool DBusMenuModel::canFetchMore(const QModelIndex &parent) const
{
    if (const auto item = itemByIndex(parent)) {
        return item->isSubmenu() && item->childItems().isEmpty();
    } else {
        return false;
    }
}

void DBusMenuModel::fetchMore(const QModelIndex &parent)
{
    if (auto item = itemByIndex(parent)) {
        fetchLayout(item->id());
    }
}

QString DBusMenuModel::serviceName() const
{
    return d->interface->service();
}

QString DBusMenuModel::objectPath() const
{
    return d->interface->path();
}

int DBusMenuModel::prefetchSize() const
{
    return d->prefetchSize;
}

void DBusMenuModel::setPrefetchSize(int levelCount)
{
    d->prefetchSize = levelCount;
}

void DBusMenuModel::click(const QModelIndex &index)
{
    auto item = itemByIndex(index);
    if (!item) {
        return;
    }

    sendEvent(item->id(), QStringLiteral("clicked"));
}

void DBusMenuModel::open(const QModelIndex &index)
{
    DBusMenuModelItem *item = itemByIndex(index);
    if (!item) {
        return;
    }

    auto call = d->interface->AboutToShow(item->id());
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, guard = QPointer(item)](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        if (!guard) {
            return;
        }

        QDBusPendingReply<bool> reply = *watcher;
        if (reply.isError()) {
            qCWarning(DBUSMENUQT) << "AboutToShow() failed:" << reply.error();
            // Temporary workaround for Chromium mishandling AboutToShow(id: 0).
            // https://chromium-review.googlesource.com/c/chromium/src/+/6854001
            if (guard->id() == 0) {
                fetchLayout(0);
            }
            return;
        }

        const bool needRefresh = reply.argumentAt<0>();
        if (needRefresh || guard->childCount() == 0) {
            fetchLayout(guard->id());
        }
    });

    sendEvent(item->id(), QStringLiteral("opened"));
}

void DBusMenuModel::close(const QModelIndex &index)
{
    auto item = itemByIndex(index);
    if (!item) {
        return;
    }

    sendEvent(item->id(), QStringLiteral("closed"));
}

void DBusMenuModel::onItemActivationRequested(int id, uint timeStamp)
{
    Q_UNUSED(timeStamp)
    if (auto item = findItemById(id)) {
        Q_EMIT activateRequested(index(item));
    }
}

void DBusMenuModel::onItemsPropertiesUpdated(const DBusMenuItemList &updated, const DBusMenuItemKeysList &removed)
{
    for (const DBusMenuItem &rawItem : updated) {
        if (DBusMenuModelItem *item = findItemById(rawItem.id)) {
            updateItem(item, rawItem.properties);
        }
    }

    for (const DBusMenuItemKeys &rawItem : removed) {
        if (DBusMenuModelItem *item = findItemById(rawItem.id)) {
            resetItem(item, rawItem.properties);
        }
    }
}

void DBusMenuModel::onLayoutUpdated(uint revision, int parentId)
{
    Q_UNUSED(revision)

    d->dirtyItems.insert(parentId);
    if (!d->dirtyTimer.isActive()) {
        d->dirtyTimer.start();
    }
}

DBusMenuModelItem *DBusMenuModel::createItem(int id, const QVariantMap &initialState)
{
    auto item = new DBusMenuModelItem(id, initialState);
    d->items[id] = item;
    connect(item, &QObject::destroyed, this, [this, id]() {
        d->items.erase(id);
    });
    return item;
}

DBusMenuModelItem *DBusMenuModel::findItemById(int id) const
{
    if (auto it = d->items.find(id); it != d->items.end()) {
        return it->second;
    }
    return nullptr;
}

void DBusMenuModel::updateItem(DBusMenuModelItem *item, const QVariantMap &properties)
{
    const auto dirtyRoles = item->update(properties);
    if (!dirtyRoles.isEmpty()) {
        const QModelIndex itemIndex = index(item);
        Q_EMIT dataChanged(itemIndex, itemIndex, dirtyRoles);
    }
}

void DBusMenuModel::resetItem(DBusMenuModelItem *item, const QStringList &keys)
{
    const auto dirtyRoles = item->reset(keys);
    if (!dirtyRoles.isEmpty()) {
        const QModelIndex itemIndex = index(item);
        Q_EMIT dataChanged(itemIndex, itemIndex, dirtyRoles);
    }
}

DBusMenuModelItem *DBusMenuModel::itemByIndex(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return d->rootItem.get();
    } else {
        return static_cast<DBusMenuModelItem *>(index.internalPointer());
    }
}

void DBusMenuModel::createSubTree(DBusMenuModelItem *parentItem, const DBusMenuLayoutItem &rawItem, int parentRow)
{
    const QModelIndex parentIndex = index(parentItem);

    beginInsertRows(parentIndex, parentRow, parentRow);
    auto childItem = createItem(rawItem.id, rawItem.properties);
    parentItem->insertChild(childItem, parentRow);
    endInsertRows();

    for (const auto &rawChildItem : rawItem.children) {
        createSubTree(childItem, rawChildItem, childItem->childCount());
    }
}

void DBusMenuModel::updateSubTree(DBusMenuModelItem *item, const DBusMenuLayoutItem &rawItem, int maxDepth)
{
    updateItem(item, rawItem.properties);

    if (maxDepth == 0) {
        return;
    }

    for (int targetRow = 0; targetRow < rawItem.children.size(); ++targetRow) {
        const auto &rawChildItem = rawItem.children[targetRow];

        if (auto childItem = findItemById(rawChildItem.id)) {
            if (childItem->parentItem() != item) {
                qCWarning(DBUSMENUQT) << "A submenu unexpectedly migrated to another parent menu";
                continue;
            }

            updateSubTree(childItem, rawChildItem, maxDepth - 1);

            const int sourceRow = item->childItems().indexOf(childItem);
            if (targetRow != sourceRow) {
                const QModelIndex parentIndex = index(item);
                beginMoveRows(parentIndex, sourceRow, sourceRow, parentIndex, sourceRow < targetRow ? targetRow + 1 : targetRow);
                item->moveChild(sourceRow, targetRow);
                endMoveRows();
            }
        } else {
            createSubTree(item, rawChildItem, targetRow);
        }
    }

    const auto childItems = item->childItems();
    for (const auto childItem : childItems) {
        const bool alive = std::any_of(rawItem.children.begin(), rawItem.children.end(), [childItem](const DBusMenuLayoutItem &other) {
            return other.id == childItem->id();
        });
        if (!alive) {
            pruneSubTree(item, childItem);
        }
    }
}

void DBusMenuModel::pruneSubTree(DBusMenuModelItem *parentItem, DBusMenuModelItem *childItem)
{
    const auto grandChildItems = childItem->childItems();
    for (const auto grandChildItem : grandChildItems) {
        pruneSubTree(childItem, grandChildItem);
    }

    const QModelIndex parentIndex = index(parentItem);
    const int row = parentItem->childItems().indexOf(childItem);

    beginRemoveRows(parentIndex, row, row);
    parentItem->removeChild(childItem);
    delete childItem;
    endRemoveRows();
}

void DBusMenuModel::fetchLayout(int id)
{
    const int maxDepth = d->prefetchSize;
    QDBusPendingReply<uint, DBusMenuLayoutItem> call = d->interface->GetLayout(id, maxDepth, QStringList());
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, id, maxDepth](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();

        QDBusPendingReply<uint, DBusMenuLayoutItem> reply = *watcher;
        if (!reply.isValid()) {
            qCWarning(DBUSMENUQT) << "GetLayout() failed:" << reply.error();
            return;
        }

        if (DBusMenuModelItem *item = findItemById(id)) {
            updateSubTree(item, reply.argumentAt<1>(), maxDepth);
        }
    });
}

void DBusMenuModel::sendEvent(int id, const QString &eventId)
{
    d->interface->Event(id, eventId, QDBusVariant(QString()), 0u);
}

#include "moc_dbusmenumodel.cpp"
