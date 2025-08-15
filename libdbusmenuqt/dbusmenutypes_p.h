/* This file is part of the dbusmenu-qt library
    SPDX-FileCopyrightText: 2009 Canonical
    SPDX-FileContributor: Aurelien Gateau <aurelien.gateau@canonical.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "dbusmenuqt_export.h"

// Qt
#include <QList>
#include <QStringList>
#include <QVariant>

class QDBusArgument;

//// DBusMenuItem
/**
 * Internal struct used to communicate on DBus
 */
struct DBusMenuItem {
    int id;
    QVariantMap properties;
};

DBUSMENUQT_EXPORT QDBusArgument &operator<<(QDBusArgument &argument, const DBusMenuItem &item);
DBUSMENUQT_EXPORT const QDBusArgument &operator>>(const QDBusArgument &argument, DBusMenuItem &item);

typedef QList<DBusMenuItem> DBusMenuItemList;

//// DBusMenuItemKeys
/**
 * Represents a list of keys for a menu item
 */
struct DBusMenuItemKeys {
    int id;
    QStringList properties;
};

DBUSMENUQT_EXPORT QDBusArgument &operator<<(QDBusArgument &argument, const DBusMenuItemKeys &);
DBUSMENUQT_EXPORT const QDBusArgument &operator>>(const QDBusArgument &argument, DBusMenuItemKeys &);

typedef QList<DBusMenuItemKeys> DBusMenuItemKeysList;

//// DBusMenuLayoutItem
/**
 * Represents an item with its children. GetLayout() returns a
 * DBusMenuLayoutItemList.
 */
struct DBusMenuLayoutItem;
struct DBusMenuLayoutItem {
    int id;
    QVariantMap properties;
    QList<DBusMenuLayoutItem> children;
};

DBUSMENUQT_EXPORT QDBusArgument &operator<<(QDBusArgument &argument, const DBusMenuLayoutItem &);
DBUSMENUQT_EXPORT const QDBusArgument &operator>>(const QDBusArgument &argument, DBusMenuLayoutItem &);

typedef QList<DBusMenuLayoutItem> DBusMenuLayoutItemList;

//// DBusMenuShortcut

class DBusMenuShortcut;

DBUSMENUQT_EXPORT QDBusArgument &operator<<(QDBusArgument &argument, const DBusMenuShortcut &);
DBUSMENUQT_EXPORT const QDBusArgument &operator>>(const QDBusArgument &argument, DBusMenuShortcut &);

DBUSMENUQT_EXPORT void DBusMenuTypes_register();