/********************************************************************
Copyright 2016  Eike Hein <hein@kde.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) version 3, or any
later version accepted by the membership of KDE e.V. (or its
successor approved by the membership of KDE e.V.), which shall
act as a proxy defined in Section 6 of version 3 of the license.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

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
