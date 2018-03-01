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
