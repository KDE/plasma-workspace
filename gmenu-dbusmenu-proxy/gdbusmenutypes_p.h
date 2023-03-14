/*
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QDBusSignature>
#include <QList>
#include <QMap>
#include <QVariant>

class QDBusArgument;

// Various
using VariantMapList = QList<QVariantMap>;

using StringBoolMap = QMap<QString, bool>;

// Menu item itself (Start method)
struct GMenuItem {
    uint id;
    uint section;
    VariantMapList items;
};

QDBusArgument &operator<<(QDBusArgument &argument, const GMenuItem &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuItem &item);

using GMenuItemList = QList<GMenuItem>;

// Information about what section or submenu to use for a particular entry
struct GMenuSection {
    uint subscription;
    uint menu;
};

QDBusArgument &operator<<(QDBusArgument &argument, const GMenuSection &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuSection &item);

// Changes of a menu item (Changed signal)
struct GMenuChange {
    uint subscription;
    uint menu;

    uint changePosition;
    uint itemsToRemoveCount;
    VariantMapList itemsToInsert;
};

QDBusArgument &operator<<(QDBusArgument &argument, const GMenuChange &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuChange &item);

using GMenuChangeList = QList<GMenuChange>;

// An application action
struct GMenuAction {
    bool enabled;
    QDBusSignature signature;
    QVariantList state;
};

QDBusArgument &operator<<(QDBusArgument &argument, const GMenuAction &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuAction &item);

using GMenuActionMap = QMap<QString, GMenuAction>;

struct GMenuActionsChange {
    QStringList removed;
    QMap<QString, bool> enabledChanged;
    QVariantMap stateChanged;
    GMenuActionMap added;
};

QDBusArgument &operator<<(QDBusArgument &argument, const GMenuActionsChange &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuActionsChange &item);

void GDBusMenuTypes_register();
