/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

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
