/*
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "utils.h"

int Utils::treeStructureToInt(int subscription, int section, int index)
{
    return subscription * 1000000 + section * 1000 + index;
}

void Utils::intToTreeStructure(int source, int &subscription, int &section, int &index)
{
    // TODO some better math :) or bit shifting or something
    index = source % 1000;
    section = (source / 1000) % 1000;
    subscription = source / 1000000;
}

QString Utils::itemActionName(const QVariantMap &item)
{
    QString actionName = item.value(QStringLiteral("action")).toString();
    if (actionName.isEmpty()) {
        actionName = item.value(QStringLiteral("submenu-action")).toString();
    }
    return actionName;
}
