/*
 * Copyright (C) 2015 Kai Uwe Broulik <kde@privat.broulik.de>
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

#include "tasksproxymodel.h"

#include "host.h"
#include "task.h"

using namespace SystemTray;

TasksProxyModel::TasksProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    connect(this, &QSortFilterProxyModel::rowsInserted, this, &TasksProxyModel::countChanged);
    connect(this, &QSortFilterProxyModel::rowsRemoved, this, &TasksProxyModel::countChanged);
}

Host *TasksProxyModel::host() const
{
    return m_host;
}

void TasksProxyModel::setHost(Host *host)
{
    if (m_host != host) {
        if (m_host) {
            disconnect(m_host, 0, this, 0);
        }

        m_host = host;
        emit hostChanged();

        if (m_host) {
            setSourceModel(m_host->allTasks());

            connect(m_host, &Host::taskStatusChanged, this, &TasksProxyModel::invalidateFilter);
            connect(m_host, &Host::shownCategoriesChanged, this, &TasksProxyModel::invalidateFilter);
            connect(m_host, &Host::showAllItemsChanged, this, &TasksProxyModel::reset);
            connect(m_host, &Host::forcedHiddenItemsChanged, this, &TasksProxyModel::reset);
            connect(m_host, &Host::forcedShownItemsChanged, this, &TasksProxyModel::reset);
        }

        invalidateFilter();
    }
}

TasksProxyModel::Category TasksProxyModel::category() const
{
    return m_category;
}

void TasksProxyModel::setCategory(Category category)
{
    if (m_category != category) {
        m_category = category;
        emit categoryChanged();

        invalidateFilter();
    }
}

void TasksProxyModel::reset()
{
    beginResetModel();
    invalidateFilter();
    endResetModel();
}

bool TasksProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent);

    if (!m_host || m_category == Category::NoTasksCategory) {
        return false;
    }

    const QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0);
    Task *task = qvariant_cast<Task *>(sourceModel()->data(sourceIndex, Qt::UserRole));

    if (!task) {
        return false;
    }

    if (!m_host->isCategoryShown(task->category())) {
        return false;
    }

    if (m_host->showAllItems() && m_category == Category::HiddenTasksCategory) {
        return false;
    }

    if (!m_host->showAllItems()) {
        if (m_category == Category::HiddenTasksCategory) {
            return !showTask(task);
        } else if (m_category == Category::ShownTasksCategory) {
            return showTask(task);
        }
    }

    return true;
}

bool TasksProxyModel::showTask(Task *task) const
{
    const QString &taskId = task->taskId();

    // not forced hidden, and (forced shown or (shown and not passive))
    return ((task->shown() && task->status() != SystemTray::Task::Passive)
           || (m_host->forcedShownItems().contains(taskId)))
           && !m_host->forcedHiddenItems().contains(taskId);
}
