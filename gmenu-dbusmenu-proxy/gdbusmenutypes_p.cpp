/*
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "gdbusmenutypes_p.h"

#include <QDBusArgument>
#include <QDBusMetaType>

// GMenuItem
QDBusArgument &operator<<(QDBusArgument &argument, const GMenuItem &item)
{
    argument.beginStructure();
    argument << item.id << item.section << item.items;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuItem &item)
{
    argument.beginStructure();
    argument >> item.id >> item.section >> item.items;
    argument.endStructure();
    return argument;
}

// GMenuSection
QDBusArgument &operator<<(QDBusArgument &argument, const GMenuSection &item)
{
    argument.beginStructure();
    argument << item.subscription << item.menu;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuSection &item)
{
    argument.beginStructure();
    argument >> item.subscription >> item.menu;
    argument.endStructure();
    return argument;
}

// GMenuChange
QDBusArgument &operator<<(QDBusArgument &argument, const GMenuChange &item)
{
    argument.beginStructure();
    argument << item.subscription << item.menu << item.changePosition << item.itemsToRemoveCount << item.itemsToInsert;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuChange &item)
{
    argument.beginStructure();
    argument >> item.subscription >> item.menu >> item.changePosition >> item.itemsToRemoveCount >> item.itemsToInsert;
    argument.endStructure();
    return argument;
}

// GMenuActionProperty
QDBusArgument &operator<<(QDBusArgument &argument, const GMenuAction &item)
{
    argument.beginStructure();
    argument << item.enabled << item.signature << item.state;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuAction &item)
{
    argument.beginStructure();
    argument >> item.enabled >> item.signature >> item.state;
    argument.endStructure();
    return argument;
}

// GMenuActionsChange
QDBusArgument &operator<<(QDBusArgument &argument, const GMenuActionsChange &item)
{
    argument.beginStructure();
    argument << item.removed << item.enabledChanged << item.stateChanged << item.added;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuActionsChange &item)
{
    argument.beginStructure();
    argument >> item.removed >> item.enabledChanged >> item.stateChanged >> item.added;
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

    qDBusRegisterMetaType<GMenuSection>();

    qDBusRegisterMetaType<GMenuChange>();
    qDBusRegisterMetaType<GMenuChangeList>();

    qDBusRegisterMetaType<GMenuAction>();
    qDBusRegisterMetaType<GMenuActionMap>();

    qDBusRegisterMetaType<GMenuActionsChange>();
    qDBusRegisterMetaType<StringBoolMap>();

    registered = true;
}
