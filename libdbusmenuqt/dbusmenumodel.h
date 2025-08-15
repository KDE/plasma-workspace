/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "dbusmenuqt_export.h"

#include <QAbstractItemModel>

class DBusMenuInterface;
class DBusMenuModelItem;
class DBusMenuModelPrivate;
struct DBusMenuItem;
struct DBusMenuLayoutItem;
struct DBusMenuItemKeys;
typedef QList<DBusMenuItem> DBusMenuItemList;
typedef QList<DBusMenuItemKeys> DBusMenuItemKeysList;

/**
 * The DBusMenuModel type provides a tree-like model representing an application menu. The imported
 * dbus menu has the com.canonical.dbusmenu interface.
 *
 * Example usage
 *
 * @code
 * auto model = new DBusMenuModel(serviceName, objectPath);
 * model->setPrefetchSize(2);
 * model->open(QModelIndex());
 *
 * auto treeView = new QTreeView();
 * treeView->setModel(model);
 * @endcode
 *
 * @note This model never emits the dataChanged() signal with an empty roles list.
 */
class DBUSMENUQT_EXPORT DBusMenuModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Roles {
        EnabledRole = Qt::UserRole + 1,
        VisibleRole,
        CheckedRole,
        SubmenuRole,
        SeparatorRole,
        ToggleTypeRole,
        ShortcutRole,
    };

    DBusMenuModel(const QString &serviceName, const QString &objectPath, QObject *parent = nullptr);
    ~DBusMenuModel() override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

    /**
     * The service name of the dbus menu object.
     */
    QString serviceName() const;

    /**
     * The path of the dbus menu object.
     */
    QString objectPath() const;

    /**
     * Returns how many levels of the layout are fetched at a time. Values greater than 1 can
     * improve the responsiveness while moving the pointer at the cost of higher amount of d-bus traffic
     * and memory usage.
     */
    int prefetchSize() const;
    void setPrefetchSize(int levelCount);

Q_SIGNALS:
    /**
     * This signal is emitted when the client that displays the global menu shows a menu with
     * the specified @a index.
     */
    void activateRequested(const QModelIndex &index);

public Q_SLOTS:
    /**
     * Notifies the application that the user has clicked an action with the specified @a index.
     */
    void click(const QModelIndex &index);

    /**
     * Notifies the application that a menu with the specified @a index has been opened.
     */
    void open(const QModelIndex &index);

    /**
     * Notifies the application that a menu with the specified @a index has been closed by the user.
     */
    void close(const QModelIndex &index);

private:
    void onItemActivationRequested(int id, uint timeStamp);
    void onItemsPropertiesUpdated(const DBusMenuItemList &updated, const DBusMenuItemKeysList &removed);
    void onLayoutUpdated(uint revision, int parentId);

    void fetchLayout(int id);
    void sendEvent(int id, const QString &eventId);

    QModelIndex index(DBusMenuModelItem *item) const;
    DBusMenuModelItem *createItem(int id, const QVariantMap &initialState = QVariantMap());
    DBusMenuModelItem *findItemById(int id) const;
    void updateItem(DBusMenuModelItem *item, const QVariantMap &properties);
    void resetItem(DBusMenuModelItem *item, const QStringList &keys);

    DBusMenuModelItem *itemByIndex(const QModelIndex &index) const;

    void createSubTree(DBusMenuModelItem *parentItem, const DBusMenuLayoutItem &rawItem, int parentRow);
    void updateSubTree(DBusMenuModelItem *item, const DBusMenuLayoutItem &rawItem, int maxDepth);
    void pruneSubTree(DBusMenuModelItem *parentItem, DBusMenuModelItem *childItem);

    std::unique_ptr<DBusMenuModelPrivate> d;
};
