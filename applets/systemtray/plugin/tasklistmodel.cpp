/*
 * Copyright (C) 2014  David Edmundson <david@davidedmundson.co.uk>
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

#include "tasklistmodel.h"

#include "task.h"

#include <QDebug>

using namespace SystemTray;

static QHash<Task::Category, int> s_taskWeights;

bool taskLessThan(const Task *lhs, const Task *rhs)
{
    /* Sorting of systemtray icons
     *
     * We sort (and thus group) in the following order, from high to low priority
     * - Notifications always comes first
     * - Category
     * - Name
     */

    const QLatin1String _not = QLatin1String("org.kde.plasma.notifications");
    if (lhs->taskId() == _not) {
        return true;
    }
    if (rhs->taskId() == _not) {
        return false;
    }

    if (lhs->category() != rhs->category()) {

        if (s_taskWeights.isEmpty()) {
            s_taskWeights.insert(Task::Communications, 0);
            s_taskWeights.insert(Task::SystemServices, 1);
            s_taskWeights.insert(Task::Hardware, 2);
            s_taskWeights.insert(Task::ApplicationStatus, 3);
            s_taskWeights.insert(Task::UnknownCategory, 4);
        }
        return s_taskWeights.value(lhs->category()) < s_taskWeights.value(rhs->category());
    }

    return lhs->name() < rhs->name();
}

TaskListModel::TaskListModel(QObject *parent):
    QAbstractListModel(parent)
{
}

QVariant TaskListModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::UserRole && index.row() >=0 && index.row() < m_tasks.count()) {
        return QVariant::fromValue(m_tasks.at(index.row()));
    }
    return QVariant();
}

int TaskListModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return m_tasks.size();
    } else {
        return 0;
    }
}

QHash< int, QByteArray > SystemTray::TaskListModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames.insert(Qt::UserRole, "modelData");
    return roleNames;
}

QList< Task* > TaskListModel::tasks() const
{
    return m_tasks;
}

void SystemTray::TaskListModel::addTask(Task *task)
{
    if (!m_tasks.contains(task)) {
        auto it = qLowerBound(m_tasks.begin(), m_tasks.end(), task, taskLessThan);
        int index = it - m_tasks.begin();
        beginInsertRows(QModelIndex(), index, index);
        m_tasks.insert(it, task);
        endInsertRows();
    }
}

void SystemTray::TaskListModel::removeTask(Task *task)
{
    int index = m_tasks.indexOf(task);
    if (index >= 0 ) {
        beginRemoveRows(QModelIndex(), index, index);
        m_tasks.removeAt(index);
        endRemoveRows();
    }
}


#include "tasklistmodel.moc"
