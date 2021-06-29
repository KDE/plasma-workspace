/*
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QString>
#include <QVariantMap>

namespace Utils
{
int treeStructureToInt(int subscription, int section, int index);
void intToTreeStructure(int source, int &subscription, int &section, int &index);

QString itemActionName(const QVariantMap &item);

}
