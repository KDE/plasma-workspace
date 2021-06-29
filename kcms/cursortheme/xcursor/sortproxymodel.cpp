/*
    SPDX-FileCopyrightText: 2006-2007 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#include "sortproxymodel.h"
#include "cursortheme.h"
#include <QString>

QHash<int, QByteArray> SortProxyModel::roleNames() const
{
    QHash<int, QByteArray> roleNames = QSortFilterProxyModel::roleNames();
    roleNames[CursorTheme::DisplayDetailRole] = "description";

    return roleNames;
}

int SortProxyModel::compare(const QModelIndex &left, const QModelIndex &right, int role) const
{
    const QAbstractItemModel *model = sourceModel();

    QString first = model->data(left, role).toString();
    QString second = model->data(right, role).toString();

    if (filterCaseSensitivity() == Qt::CaseInsensitive) {
        first = first.toLower();
        second = second.toLower();
    }

    return QString::localeAwareCompare(first, second);
}

bool SortProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const int result = compare(left, right, Qt::DisplayRole);

    if (result != 0)
        return (result < 0);
    else
        return compare(left, right, CursorTheme::DisplayDetailRole) < 0;
}
