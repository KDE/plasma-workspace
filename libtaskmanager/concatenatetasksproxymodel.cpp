/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "concatenatetasksproxymodel.h"

namespace TaskManager
{
ConcatenateTasksProxyModel::ConcatenateTasksProxyModel(QObject *parent)
    : QConcatenateTablesProxyModel(parent)
{
}

ConcatenateTasksProxyModel::~ConcatenateTasksProxyModel()
{
}

QModelIndex ConcatenateTasksProxyModel::mapIfaceToSource(const QModelIndex &index) const
{
    return mapToSource(index);
}

}
