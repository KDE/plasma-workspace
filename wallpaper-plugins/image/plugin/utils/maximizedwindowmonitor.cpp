/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "maximizedwindowmonitor.h"

#include "abstracttasksmodel.h" // For enums
#include "activityinfo.h"
#include "virtualdesktopinfo.h"

MaximizedWindowMonitor::MaximizedWindowMonitor(QObject *parent)
    : TaskManager::TasksModel(parent)
    , m_activityInfo(activityInfo())
    , m_virtualDesktopInfo(virtualDesktopInfo())
{
    setSortMode(SortMode::SortActivity);
    setGroupMode(GroupMode::GroupDisabled);

    auto currentActivity = std::bind(&TaskManager::ActivityInfo::currentActivity, m_activityInfo);
    auto setCurrentActivity = std::bind(&TaskManager::TasksModel::setActivity, this, currentActivity);
    setCurrentActivity();
    connect(m_activityInfo.get(), &TaskManager::ActivityInfo::currentActivityChanged, this, setCurrentActivity);

    auto currentDesktop = std::bind(&TaskManager::VirtualDesktopInfo::currentDesktop, m_virtualDesktopInfo);
    auto setCurrentDesktop = std::bind(&TaskManager::TasksModel::setVirtualDesktop, this, currentDesktop);
    setCurrentDesktop();
    connect(m_virtualDesktopInfo.get(), &TaskManager::VirtualDesktopInfo::currentDesktopChanged, this, setCurrentDesktop);

    setFilterMinimized(true);
    setFilterByActivity(true);
    setFilterByVirtualDesktop(true);
    setFilterByRegion(RegionFilterMode::Mode::Intersect);
}

MaximizedWindowMonitor::~MaximizedWindowMonitor()
{
}

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
