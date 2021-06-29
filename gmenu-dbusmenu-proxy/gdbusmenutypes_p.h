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
Q_DECLARE_METATYPE(VariantMapList);

using StringBoolMap = QMap<QString, bool>;
Q_DECLARE_METATYPE(StringBoolMap);

// Menu item itself (Start method)
struct GMenuItem {
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
struct GMenuSection {
    uint subscription;
    uint menu;
};
Q_DECLARE_METATYPE(GMenuSection);

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
Q_DECLARE_METATYPE(GMenuChange);

QDBusArgument &operator<<(QDBusArgument &argument, const GMenuChange &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuChange &item);

using GMenuChangeList = QList<GMenuChange>;
Q_DECLARE_METATYPE(GMenuChangeList);

// An application action
struct GMenuAction {
    bool enabled;
    QDBusSignature signature;
    QVariantList state;
};
Q_DECLARE_METATYPE(GMenuAction);

QDBusArgument &operator<<(QDBusArgument &argument, const GMenuAction &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuAction &item);

using GMenuActionMap = QMap<QString, GMenuAction>;
Q_DECLARE_METATYPE(GMenuActionMap);

struct GMenuActionsChange {
    QStringList removed;
    QMap<QString, bool> enabledChanged;
    QVariantMap stateChanged;
    GMenuActionMap added;
};
Q_DECLARE_METATYPE(GMenuActionsChange);

QDBusArgument &operator<<(QDBusArgument &argument, const GMenuActionsChange &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, GMenuActionsChange &item);

void GDBusMenuTypes_register();
