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

#include "abstracttasksmodel.h"

#include <QMetaEnum>

namespace TaskManager
{
AbstractTasksModel::AbstractTasksModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

AbstractTasksModel::~AbstractTasksModel()
{
}

QHash<int, QByteArray> AbstractTasksModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();

    QMetaEnum e = metaObject()->enumerator(metaObject()->indexOfEnumerator("AdditionalRoles"));

    for (int i = 0; i < e.keyCount(); ++i) {
        roles.insert(e.value(i), e.key(i));
    }

    return roles;
}

QModelIndex AbstractTasksModel::index(int row, int column, const QModelIndex &parent) const
{
    return hasIndex(row, column, parent) ? createIndex(row, column, nullptr) : QModelIndex();
}

void AbstractTasksModel::requestActivate(const QModelIndex &index)
{
    Q_UNUSED(index)
}

void AbstractTasksModel::requestNewInstance(const QModelIndex &index)
{
    Q_UNUSED(index)
}

void AbstractTasksModel::requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls)
{
    Q_UNUSED(index)
    Q_UNUSED(urls)
}

void AbstractTasksModel::requestClose(const QModelIndex &index)
{
    Q_UNUSED(index)
}

void AbstractTasksModel::requestMove(const QModelIndex &index)
{
    Q_UNUSED(index)
}

void AbstractTasksModel::requestResize(const QModelIndex &index)
{
    Q_UNUSED(index)
}

void AbstractTasksModel::requestToggleMinimized(const QModelIndex &index)
{
    Q_UNUSED(index)
}

void AbstractTasksModel::requestToggleMaximized(const QModelIndex &index)
{
    Q_UNUSED(index)
}

void AbstractTasksModel::requestToggleKeepAbove(const QModelIndex &index)
{
    Q_UNUSED(index)
}

void AbstractTasksModel::requestToggleKeepBelow(const QModelIndex &index)
{
    Q_UNUSED(index)
}

void AbstractTasksModel::requestToggleFullScreen(const QModelIndex &index)
{
    Q_UNUSED(index)
}

void AbstractTasksModel::requestToggleShaded(const QModelIndex &index)
{
    Q_UNUSED(index)
}

void AbstractTasksModel::requestVirtualDesktops(const QModelIndex &index, const QVariantList &desktops)
{
    Q_UNUSED(index)
    Q_UNUSED(desktops)
}

void AbstractTasksModel::requestNewVirtualDesktop(const QModelIndex &index)
{
    Q_UNUSED(index)
}

void AbstractTasksModel::requestActivities(const QModelIndex &index, const QStringList &activities)
{
    Q_UNUSED(index)
    Q_UNUSED(activities)
}

void AbstractTasksModel::requestPublishDelegateGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate)
{
    Q_UNUSED(index)
    Q_UNUSED(geometry)
    Q_UNUSED(delegate)
}

}
