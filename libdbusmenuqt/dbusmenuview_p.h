/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "dbusmenuview.h"

class DBusMenuViewPrivate
{
public:
    DBusMenuModel *model = nullptr;
    QPersistentModelIndex rootIndex;
    bool rootIndexValid = false;
    QHash<QPersistentModelIndex, QAction *> actions;
};
