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

class FlattenTaskGroupsProxyModel::Private
{
public:
    Private(FlattenTaskGroupsProxyModel *q);

    AbstractTasksModelIface *sourceTasksModel = nullptr;

private:
    FlattenTaskGroupsProxyModel *q;
};

FlattenTaskGroupsProxyModel::Private::Private(FlattenTaskGroupsProxyModel *q)
    : q(q)
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

void FlattenTaskGroupsProxyModel::requestActivate(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestActivate(mapToSource(index));
    }
}

void FlattenTaskGroupsProxyModel::requestNewInstance(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestNewInstance(mapToSource(index));
    }
}

void FlattenTaskGroupsProxyModel::requestClose(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestClose(mapToSource(index));
    }
}

void FlattenTaskGroupsProxyModel::requestMove(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestMove(mapToSource(index));
    }
}

void FlattenTaskGroupsProxyModel::requestResize(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestResize(mapToSource(index));
    }
}

void FlattenTaskGroupsProxyModel::requestToggleMinimized(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestToggleMinimized(mapToSource(index));
    }
}

void FlattenTaskGroupsProxyModel::requestToggleMaximized(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestToggleMaximized(mapToSource(index));
    }
}

void FlattenTaskGroupsProxyModel::requestToggleKeepAbove(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestToggleKeepAbove(mapToSource(index));
    }
}

void FlattenTaskGroupsProxyModel::requestToggleKeepBelow(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestToggleKeepBelow(mapToSource(index));
    }
}

void FlattenTaskGroupsProxyModel::requestToggleFullScreen(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestToggleFullScreen(mapToSource(index));
    }
}

void FlattenTaskGroupsProxyModel::requestToggleShaded(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestToggleShaded(mapToSource(index));
    }
}

void FlattenTaskGroupsProxyModel::requestVirtualDesktop(const QModelIndex &index, qint32 desktop)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestVirtualDesktop(mapToSource(index), desktop);
    }
}

void FlattenTaskGroupsProxyModel::requestPublishDelegateGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate)
{
    if (index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestPublishDelegateGeometry(mapToSource(index), geometry, delegate);
    }
}

}
