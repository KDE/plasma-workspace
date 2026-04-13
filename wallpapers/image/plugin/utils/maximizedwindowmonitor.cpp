/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "maximizedwindowmonitor.h"

#include "abstracttasksmodel.h" // For enums
#include "activityinfo.h"

MaximizedWindowMonitor::MaximizedWindowMonitor(QObject *parent)
    : TaskManager::TasksModel(parent)
    , m_activityInfo(activityInfo())
{
    setSortMode(SortMode::SortActivity);
    setGroupMode(GroupMode::GroupDisabled);

    auto currentActivity = std::bind(&TaskManager::ActivityInfo::currentActivity, m_activityInfo);
    auto setCurrentActivity = std::bind(&TaskManager::TasksModel::setActivity, this, currentActivity);
    setCurrentActivity();
    connect(m_activityInfo.get(), &TaskManager::ActivityInfo::currentActivityChanged, this, setCurrentActivity);

    setFilterMinimized(true);
    setFilterByActivity(true);
    setFilterByCurrentVirtualDesktop(true);
    setFilterByRegion(RegionFilterMode::Mode::Intersect);
}

MaximizedWindowMonitor::~MaximizedWindowMonitor() = default;

bool MaximizedWindowMonitor::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0);

    if (!TaskManager::TasksModel::filterAcceptsRow(sourceRow, sourceParent)) {
        return false;
    }

    if (sourceIndex.data(TaskManager::AbstractTasksModel::IsMaximized).toBool() || sourceIndex.data(TaskManager::AbstractTasksModel::IsFullScreen).toBool()) {
        return true;
    }

    return false;
}
