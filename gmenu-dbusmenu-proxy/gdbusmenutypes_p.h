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

#include <QHash>
#include <QList>
#include <QVariant>

class QDBusArgument;

// Various
using VariantHashList = QList<QVariantHash>;

// Menu item itself (Start method)
struct GMenuItem
{
    uint id;
    uint count;
    VariantHashList items;
};
Q_DECLARE_METATYPE(GMenuItem)

QDBusArgument &operator<<(QDBusArgument &argument, const GMenuItem &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuItem &item);

using GMenuItemList = QList<GMenuItem>;

// Changes of a menu item (Changed signal)
struct GMenuChange
{
    uint id;
    uint count;
    uint changePosition;
    uint itemsToRemoveCount;
    VariantHashList itemsToInsert;
};
Q_DECLARE_METATYPE(GMenuChange)

QDBusArgument &operator<<(QDBusArgument &argument, const GMenuChange &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuChange &item);

using GMenuChangeList = QList<GMenuChange>;

void GDBusMenuTypes_register();
