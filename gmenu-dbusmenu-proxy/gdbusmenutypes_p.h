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

#pragma once

#include <QDBusSignature>
#include <QList>
#include <QMap>
#include <QVariant>

class QDBusArgument;

// Various
using VariantMapList = QList<QVariantMap>;
Q_DECLARE_METATYPE(VariantMapList);

using StringBoolMap = QMap<QString, bool>;
Q_DECLARE_METATYPE(StringBoolMap);

// Menu item itself (Start method)
struct GMenuItem
{
    uint id;
    uint section;
    VariantMapList items;
};
Q_DECLARE_METATYPE(GMenuItem);

QDBusArgument &operator<<(QDBusArgument &argument, const GMenuItem &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuItem &item);

using GMenuItemList = QList<GMenuItem>;
Q_DECLARE_METATYPE(GMenuItemList);

// Information about what section or submenu to use for a particular entry
struct GMenuSection
{
    uint subscription;
    uint menu;
};
Q_DECLARE_METATYPE(GMenuSection);

QDBusArgument &operator<<(QDBusArgument &argument, const GMenuSection &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuSection &item);

// Changes of a menu item (Changed signal)
struct GMenuChange
{
    uint subscription;
    uint menu;

    uint changePosition;
    uint itemsToRemoveCount;
    VariantMapList itemsToInsert;
};
Q_DECLARE_METATYPE(GMenuChange);

QDBusArgument &operator<<(QDBusArgument &argument, const GMenuChange &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuChange &item);

using GMenuChangeList = QList<GMenuChange>;
Q_DECLARE_METATYPE(GMenuChangeList);

// An application action
struct GMenuAction
{
    bool enabled;
    QDBusSignature signature;
    QVariantList state;
};
Q_DECLARE_METATYPE(GMenuAction);

QDBusArgument &operator<<(QDBusArgument &argument, const GMenuAction &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuAction &item);

using GMenuActionMap = QMap<QString, GMenuAction>;
Q_DECLARE_METATYPE(GMenuActionMap);

struct GMenuActionsChange
{
    QStringList removed;
    QMap<QString, bool> enabledChanged;
    QVariantMap stateChanged;
    GMenuActionMap added;
};
Q_DECLARE_METATYPE(GMenuActionsChange);

QDBusArgument &operator<<(QDBusArgument &argument, const GMenuActionsChange &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuActionsChange &item);

void GDBusMenuTypes_register();
