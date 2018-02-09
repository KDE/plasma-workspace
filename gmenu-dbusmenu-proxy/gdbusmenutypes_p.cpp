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

#include "gdbusmenutypes_p.h"

#include <QDBusArgument>
#include <QDBusMetaType>

// GMenuItem
QDBusArgument &operator<<(QDBusArgument &argument, const GMenuItem &item)
{
    argument.beginStructure();
    argument << item.id << item.count << item.items;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuItem &item)
{
    argument.beginStructure();
    argument >> item.id >> item.count >> item.items;
    argument.endStructure();
    return argument;
}

// GMenuChange
QDBusArgument &operator<<(QDBusArgument &argument, const GMenuChange &item)
{
    argument.beginStructure();
    argument << item.id << item.count << item.changePosition << item.itemsToRemoveCount << item.itemsToInsert;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuChange &item)
{
    argument.beginStructure();
    argument >> item.id >> item.count >> item.changePosition >> item.itemsToRemoveCount >> item.itemsToInsert;
    argument.endStructure();
    return argument;
}

void GDBusMenuTypes_register()
{
    static bool registered = false;
    if (registered) {
        return;
    }

    qDBusRegisterMetaType<GMenuItem>();
    qDBusRegisterMetaType<GMenuItemList>();

    qDBusRegisterMetaType<GMenuChange>();
    qDBusRegisterMetaType<GMenuChangeList>();

    registered = true;
}
