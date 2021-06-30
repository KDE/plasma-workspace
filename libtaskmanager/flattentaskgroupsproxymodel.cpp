/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "flattentaskgroupsproxymodel.h"

namespace TaskManager
{
class Q_DECL_HIDDEN FlattenTaskGroupsProxyModel::Private
{
public:
    Private(FlattenTaskGroupsProxyModel *q);

    AbstractTasksModelIface *sourceTasksModel = nullptr;
};

FlattenTaskGroupsProxyModel::Private::Private(FlattenTaskGroupsProxyModel *)
{
}

FlattenTaskGroupsProxyModel::FlattenTaskGroupsProxyModel(QObject *parent)
    : KDescendantsProxyModel(parent)
    , d(new Private(this))
{
}

FlattenTaskGroupsProxyModel::~FlattenTaskGroupsProxyModel()
{
}

void FlattenTaskGroupsProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    d->sourceTasksModel = dynamic_cast<AbstractTasksModelIface *>(sourceModel);

    KDescendantsProxyModel::setSourceModel(sourceModel);
}

QModelIndex FlattenTaskGroupsProxyModel::mapIfaceToSource(const QModelIndex &index) const
{
    return mapToSource(index);
}

}
