/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include "tasksmodel.h"

namespace TaskManager
{
class WindowTasksModel;
class StartupTasksModel;
class LauncherTasksModel;
class ConcatenateTasksProxyModel;
class TaskFilterProxyModel;
class TaskGroupingProxyModel;
class FlattenTaskGroupsProxyModel;
class VirtualDesktopInfo;
class ActivityInfo;

class TASKMANAGER_NO_EXPORT TasksModel::Private
{
public:
    Private(TasksModel *q);
    ~Private();

    static int instanceCount;

    static WindowTasksModel *windowTasksModel;
    static StartupTasksModel *startupTasksModel;
    LauncherTasksModel *launcherTasksModel = nullptr;
    ConcatenateTasksProxyModel *concatProxyModel = nullptr;
    TaskFilterProxyModel *filterProxyModel = nullptr;
    TaskGroupingProxyModel *groupingProxyModel = nullptr;
    FlattenTaskGroupsProxyModel *flattenGroupsProxyModel = nullptr;
    AbstractTasksModelIface *abstractTasksSourceModel = nullptr;

    bool anyTaskDemandsAttention = false;

    int launcherCount = 0;

    TasksModel::SortMode sortMode = TasksModel::SortAlpha;
    bool separateLaunchers = true;
    bool launchInPlace = false;
    bool launchersEverSet = false;
    bool launcherSortingDirty = false;
    bool launcherCheckNeeded = false;
    QList<int> sortedPreFilterRows;
    QVector<int> sortRowInsertQueue;
    bool sortRowInsertQueueStale = false;
    QHash<QString, int> activityTaskCounts;
    static VirtualDesktopInfo *virtualDesktopInfo;
    static int virtualDesktopInfoUsers;
    static ActivityInfo *activityInfo;
    static int activityInfoUsers;

    bool groupInline = false;
    int groupingWindowTasksThreshold = -1;

    bool usedByQml = false;
    bool componentComplete = false;

    void initModels();
    void initLauncherTasksModel();
    void updateAnyTaskDemandsAttention();
    void updateManualSortMap();
    void consolidateManualSortMapForGroup(const QModelIndex &groupingProxyIndex);
    void updateGroupInline();
    QModelIndex preFilterIndex(const QModelIndex &sourceIndex) const;
    void updateActivityTaskCounts();
    void forceResort();
    bool lessThan(const QModelIndex &left, const QModelIndex &right, bool sortOnlyLaunchers = false) const;

private:
    TasksModel *q = nullptr;
};

class TASKMANAGER_NO_EXPORT TasksModel::TasksModelLessThan
{
public:
    inline TasksModelLessThan(const QAbstractItemModel *s, TasksModel *p, bool sortOnlyLaunchers)
        : sourceModel(s)
        , tasksModel(p)
        , sortOnlyLaunchers(sortOnlyLaunchers)
    {
    }

    inline bool operator()(int r1, int r2) const
    {
        QModelIndex i1 = sourceModel->index(r1, 0);
        QModelIndex i2 = sourceModel->index(r2, 0);
        return tasksModel->d->lessThan(i1, i2, sortOnlyLaunchers);
    }

private:
    const QAbstractItemModel *sourceModel;
    const TasksModel *tasksModel;
    bool sortOnlyLaunchers;
};

}
