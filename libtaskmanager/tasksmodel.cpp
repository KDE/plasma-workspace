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

#include "tasksmodel.h"
#include "activityinfo.h"
#include "concatenatetasksproxymodel.h"
#include "taskfilterproxymodel.h"
#include "taskgroupingproxymodel.h"
#include "tasktools.h"

#include <config-X11.h>

#include "launchertasksmodel.h"
#include "waylandtasksmodel.h"
#include "startuptasksmodel.h"
#if HAVE_X11
#include "xwindowtasksmodel.h"
#endif

#include <KWindowSystem>

#include <QGuiApplication>
#include <QTimer>
#include <QUrl>

#if HAVE_X11
#include <QX11Info>
#endif

#include <numeric>

namespace TaskManager
{

class TasksModel::Private
{
public:
    Private(TasksModel *q);
    ~Private();

    static int instanceCount;

    static AbstractTasksModel* windowTasksModel;
    static StartupTasksModel* startupTasksModel;
    LauncherTasksModel* launcherTasksModel = nullptr;
    ConcatenateTasksProxyModel* concatProxyModel = nullptr;
    TaskFilterProxyModel* filterProxyModel = nullptr;
    TaskGroupingProxyModel* groupingProxyModel = nullptr;

    bool anyTaskDemandsAttention = false;

    int virtualDesktop = -1;
    int screen = -1;
    QString activity;

    SortMode sortMode = SortAlpha;
    bool separateLaunchers = true;
    bool launchInPlace = false;
    bool launcherSortingDirty = false;
    QList<int> sortedPreFilterRows;
    QVector<int> sortRowInsertQueue;
    QHash<QString, int> activityTaskCounts;
    static ActivityInfo* activityInfo;
    static int activityInfoUsers;

    void initModels();
    void updateAnyTaskDemandsAttention();
    void updateManualSortMap();
    void syncManualSortMapForGroup(const QModelIndex &parent);
    QModelIndex preFilterIndex(const QModelIndex &sourceIndex) const;
    void updateActivityTaskCounts();
    void forceResort();
    bool lessThan(const QModelIndex &left, const QModelIndex &right,
        bool sortOnlyLaunchers = false) const;

private:
    TasksModel *q;
};

class TasksModel::TasksModelLessThan
{
public:
    inline TasksModelLessThan(const QAbstractItemModel *s, TasksModel *p, bool sortOnlyLaunchers)
        : sourceModel(s), tasksModel(p), sortOnlyLaunchers(sortOnlyLaunchers) {}

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

int TasksModel::Private::instanceCount = 0;
AbstractTasksModel* TasksModel::Private::windowTasksModel = nullptr;
StartupTasksModel* TasksModel::Private::startupTasksModel = nullptr;
ActivityInfo* TasksModel::Private::activityInfo = nullptr;
int TasksModel::Private::activityInfoUsers = 0;

TasksModel::Private::Private(TasksModel *q)
    : q(q)
{
    ++instanceCount;
}

TasksModel::Private::~Private()
{
    --instanceCount;

    if (sortMode == SortActivity) {
        --activityInfoUsers;
    }

    if (!instanceCount) {
        delete windowTasksModel;
        windowTasksModel = nullptr;
        delete startupTasksModel;
        startupTasksModel = nullptr;
        delete activityInfo;
        activityInfo = nullptr;
    }
}

void TasksModel::Private::initModels()
{
    // NOTE: Overview over the entire model chain assembled here:
    // {X11,Wayland}WindowTasksModel, StartupTasksModel, LauncherTasksModel
    //  -> ConcatenateTasksProxyModel concatenates them into a single list.
    //   -> TaskFilterProxyModel filters by state (e.g. virtual desktop).
    //    -> TaskGroupingProxyModel groups by application (we go from flat list to tree).
    //     -> TasksModel collapses top-level items into task lifecycle abstraction, sorts.

    if (!windowTasksModel && QGuiApplication::platformName().startsWith(QLatin1String("wayland"))) {
        windowTasksModel = new WaylandTasksModel();
    }

#if HAVE_X11
    if (!windowTasksModel && QX11Info::isPlatformX11()) {
        windowTasksModel = new XWindowTasksModel();
    }
#endif

    QObject::connect(windowTasksModel, &QAbstractItemModel::rowsInserted, q,
        [this]() {
            if (sortMode == SortActivity) {
                updateActivityTaskCounts();
            }
        }
    );

    QObject::connect(windowTasksModel, &QAbstractItemModel::rowsRemoved, q,
        [this]() {
            if (sortMode == SortActivity) {
                updateActivityTaskCounts();
                forceResort();
            }
        }
    );

    QObject::connect(windowTasksModel, &QAbstractItemModel::dataChanged, q,
        [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) {
            Q_UNUSED(topLeft)
            Q_UNUSED(bottomRight)

            if (sortMode == SortActivity && roles.contains(AbstractTasksModel::Activities)) {
                updateActivityTaskCounts();
            }
        }
    );

    if (!startupTasksModel) {
        startupTasksModel = new StartupTasksModel();
    }

    launcherTasksModel = new LauncherTasksModel(q);
    QObject::connect(launcherTasksModel, &LauncherTasksModel::launcherListChanged,
        q, &TasksModel::launcherListChanged);
    QObject::connect(launcherTasksModel, &QAbstractItemModel::rowsInserted,
        q, &TasksModel::launcherCountChanged);
    QObject::connect(launcherTasksModel, &QAbstractItemModel::rowsRemoved,
        q, &TasksModel::launcherCountChanged);
    QObject::connect(launcherTasksModel, &QAbstractItemModel::modelReset,
        q, &TasksModel::launcherCountChanged);

    concatProxyModel = new ConcatenateTasksProxyModel(q);

    concatProxyModel->addSourceModel(windowTasksModel);
    concatProxyModel->addSourceModel(startupTasksModel);
    concatProxyModel->addSourceModel(launcherTasksModel);

    // If we're in manual sort mode, we need to seed the sort map on pending row
    // insertions.
    QObject::connect(concatProxyModel, &QAbstractItemModel::rowsAboutToBeInserted, q,
        [this](const QModelIndex &parent, int start, int end) {
            Q_UNUSED(parent)

            if (sortMode != SortManual) {
                return;
            }

            const int delta = (end - start) + 1;
            QMutableListIterator<int> it(sortedPreFilterRows);

            while (it.hasNext()) {
                it.next();

                if (it.value() >= start) {
                    it.setValue(it.value() + delta);
                }
            }

            for (int i = start; i <= end; ++i) {
                sortedPreFilterRows.append(i);

                if (0 && !separateLaunchers) { // FIXME TODO: Disable until done.
                    sortRowInsertQueue.append(sortedPreFilterRows.count() - 1);
                }
            }
        }
    );

    // If we're in manual sort mode, we need to update the sort map on row insertions.
    QObject::connect(concatProxyModel, &QAbstractItemModel::rowsInserted, q,
        [this](const QModelIndex &parent, int start, int end) {
            Q_UNUSED(parent)
            Q_UNUSED(start)
            Q_UNUSED(end)

            if (sortMode == SortManual) {
                updateManualSortMap();
            }
        }
    );

    // If we're in manual sort mode, we need to update the sort map on row removals.
    QObject::connect(concatProxyModel, &QAbstractItemModel::rowsAboutToBeRemoved, q,
        [this](const QModelIndex &parent, int first, int last) {
            Q_UNUSED(parent)

            if (sortMode != SortManual) {
                return;
            }

            for (int i = first; i <= last; ++i) {
                sortedPreFilterRows.removeOne(i);
            }

            const int delta = (last - first) + 1;
            QMutableListIterator<int> it(sortedPreFilterRows);

            while (it.hasNext()) {
                it.next();

                if (it.value() > last) {
                    it.setValue(it.value() - delta);
                }
            }
        }
    );

    filterProxyModel = new TaskFilterProxyModel(q);
    filterProxyModel->setSourceModel(concatProxyModel);
    QObject::connect(filterProxyModel, &TaskFilterProxyModel::virtualDesktopChanged,
        q, &TasksModel::virtualDesktopChanged);
    QObject::connect(filterProxyModel, &TaskFilterProxyModel::screenChanged,
        q, &TasksModel::screenChanged);
    QObject::connect(filterProxyModel, &TaskFilterProxyModel::activityChanged,
        q, &TasksModel::activityChanged);
    QObject::connect(filterProxyModel, &TaskFilterProxyModel::filterByVirtualDesktopChanged,
        q, &TasksModel::filterByVirtualDesktopChanged);
    QObject::connect(filterProxyModel, &TaskFilterProxyModel::filterByScreenChanged,
        q, &TasksModel::filterByScreenChanged);
    QObject::connect(filterProxyModel, &TaskFilterProxyModel::filterByActivityChanged,
        q, &TasksModel::filterByActivityChanged);
    QObject::connect(filterProxyModel, &TaskFilterProxyModel::filterNotMinimizedChanged,
        q, &TasksModel::filterNotMinimizedChanged);

    groupingProxyModel = new TaskGroupingProxyModel(q);
    groupingProxyModel->setSourceModel(filterProxyModel);
    QObject::connect(groupingProxyModel, &TaskGroupingProxyModel::groupModeChanged,
        q, &TasksModel::groupModeChanged);
    QObject::connect(groupingProxyModel, &TaskGroupingProxyModel::windowTasksThresholdChanged,
        q, &TasksModel::groupingWindowTasksThresholdChanged);
    QObject::connect(groupingProxyModel, &TaskGroupingProxyModel::blacklistedAppIdsChanged,
        q, &TasksModel::groupingAppIdBlacklistChanged);
    QObject::connect(groupingProxyModel, &TaskGroupingProxyModel::blacklistedLauncherUrlsChanged,
        q, &TasksModel::groupingLauncherUrlBlacklistChanged);

    QObject::connect(groupingProxyModel, &QAbstractItemModel::rowsInserted, q,
        [this](const QModelIndex &parent, int first, int last) {
            // We can ignore group members.
            if (parent.isValid()) {
                return;
            }

            for (int i = first; i <= last; ++i) {
                const QModelIndex &sourceIndex = groupingProxyModel->index(i, 0);
                const QString &appId = sourceIndex.data(AbstractTasksModel::AppId).toString();

                if (sourceIndex.data(AbstractTasksModel::IsDemandingAttention).toBool()) {
                    updateAnyTaskDemandsAttention();
                }

                // When we get a window we have a startup for, cause the startup to be re-filtered.
                if (sourceIndex.data(AbstractTasksModel::IsWindow).toBool()) {
                    const QString &appName = sourceIndex.data(AbstractTasksModel::AppName).toString();

                    for (int i = 0; i < startupTasksModel->rowCount(); ++i) {
                        QModelIndex startupIndex = startupTasksModel->index(i, 0);

                        if (appId == startupIndex.data(AbstractTasksModel::AppId).toString()
                            || appName == startupIndex.data(AbstractTasksModel::AppName).toString()) {
                            startupTasksModel->dataChanged(startupIndex, startupIndex);
                        }
                    }
                }

                // When we get a window or startup we have a launcher for, cause the launcher to be re-filtered.
                if (sourceIndex.data(AbstractTasksModel::IsWindow).toBool()
                    || sourceIndex.data(AbstractTasksModel::IsStartup).toBool()) {
                    const QUrl &launcherUrl = sourceIndex.data(AbstractTasksModel::LauncherUrl).toUrl();

                    for (int i = 0; i < launcherTasksModel->rowCount(); ++i) {
                        QModelIndex launcherIndex = launcherTasksModel->index(i, 0);
                        const QString &launcherAppId = launcherIndex.data(AbstractTasksModel::AppId).toString();

                        if ((!appId.isEmpty() && appId == launcherAppId)
                            || launcherUrlsMatch(launcherUrl, launcherIndex.data(AbstractTasksModel::LauncherUrl).toUrl(),
                            IgnoreQueryItems)) {
                            launcherTasksModel->dataChanged(launcherIndex, launcherIndex);
                        }
                    }
                }
            }
         }
    );

    // When a window is removed, we have to trigger a re-filter of matching launchers.
    QObject::connect(groupingProxyModel, &QAbstractItemModel::rowsAboutToBeRemoved, q,
        [this](const QModelIndex &parent, int first, int last) {
            // We can ignore group members.
            if (parent.isValid()) {
                return;
            }

            for (int i = first; i <= last; ++i) {
                const QModelIndex &sourceIndex = groupingProxyModel->index(i, 0);

                if (sourceIndex.data(AbstractTasksModel::IsDemandingAttention).toBool()) {
                    updateAnyTaskDemandsAttention();
                }

                if (!sourceIndex.data(AbstractTasksModel::IsWindow).toBool()) {
                    continue;
                }

                const QUrl &launcherUrl = sourceIndex.data(AbstractTasksModel::LauncherUrl).toUrl();

                if (!launcherUrl.isEmpty() && launcherUrl.isValid()) {
                    const int pos = launcherTasksModel->launcherPosition(launcherUrl);

                    if (pos != -1) {
                        QModelIndex launcherIndex = launcherTasksModel->index(pos, 0);

                        QMetaObject::invokeMethod(launcherTasksModel, "dataChanged", Qt::QueuedConnection,
                            Q_ARG(QModelIndex, launcherIndex), Q_ARG(QModelIndex, launcherIndex));
                        QMetaObject::invokeMethod(q, "launcherCountChanged", Qt::QueuedConnection);
                    }
                }
            }
        }
    );

    // Update anyTaskDemandsAttention on source data changes.
    QObject::connect(groupingProxyModel, &QAbstractItemModel::dataChanged, q,
        [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) {
            Q_UNUSED(bottomRight)

            // We can ignore group members.
            if (topLeft.isValid()) {
                return;
            }

            if (roles.isEmpty() || roles.contains(AbstractTasksModel::IsDemandingAttention)) {
                updateAnyTaskDemandsAttention();
            }
        }
    );

    // Update anyTaskDemandsAttention on source model resets.
    QObject::connect(groupingProxyModel, &QAbstractItemModel::modelReset, q,
        [this]() { updateAnyTaskDemandsAttention(); }
    );

    q->setSourceModel(groupingProxyModel);

    QObject::connect(q, &QAbstractItemModel::rowsInserted, q,
        [this](const QModelIndex &parent, int first, int last) {
            Q_UNUSED(first)
            Q_UNUSED(last)

            q->countChanged();

            // If we're in manual sort mode, we need to consolidate new children
            // of a group in the manual sort map to prepare for when a group
            // gets dissolved.
            // This is done after we've already had a chance to sort the new child
            // in alphabetically in this proxy.
            if (sortMode == SortManual && parent.isValid()) {
                syncManualSortMapForGroup(parent);
            }
        }
    );


    QObject::connect(q, &QAbstractItemModel::rowsRemoved, q, &TasksModel::countChanged);
    QObject::connect(q, &QAbstractItemModel::modelReset, q, &TasksModel::countChanged);
}

void TasksModel::Private::updateAnyTaskDemandsAttention()
{
    bool taskFound = false;

    for (int i = 0; i < groupingProxyModel->rowCount(); ++i) {
        if (groupingProxyModel->index(i, 0).data(AbstractTasksModel::IsDemandingAttention).toBool()) {
            taskFound = true;
            break;
        }
    }

    if (taskFound != anyTaskDemandsAttention) {
        anyTaskDemandsAttention = taskFound;
        q->anyTaskDemandsAttentionChanged();
    }
}

void TasksModel::Private::updateManualSortMap()
{
    // Empty map; full sort.
    if (sortedPreFilterRows.isEmpty()) {
        sortedPreFilterRows.reserve(concatProxyModel->rowCount());

        for (int i = 0; i < concatProxyModel->rowCount(); ++i) {
            sortedPreFilterRows.append(i);
        }

        // Full sort.
        TasksModelLessThan lt(concatProxyModel, q, false);
        std::stable_sort(sortedPreFilterRows.begin(), sortedPreFilterRows.end(), lt);

        return;
    }

    // Existing map; check whether launchers need sorting by launcher list position.
    if (separateLaunchers) {
        // Sort only launchers.
        TasksModelLessThan lt(concatProxyModel, q, true);
        std::stable_sort(sortedPreFilterRows.begin(), sortedPreFilterRows.end(), lt);
    // Otherwise process any entries in the insert queue and move them intelligently
    // in the sort map.
    } else if (0) { // FIXME TODO: Disable until done.
        while (sortRowInsertQueue.count()) {
            const int row = sortRowInsertQueue.takeFirst();
            const QModelIndex &idx = concatProxyModel->index(sortedPreFilterRows.at(row), 0);

            // New launcher tasks go after the last launcher in the proxy, or to the start of
            // the map if there are none.
            if (idx.data(AbstractTasksModel::IsLauncher).toBool()) {
                int insertPos = 0;

                for (int i = 0; i < row; ++i) {
                    const QModelIndex &proxyIdx = q->index(i, 0);

                    if (proxyIdx.data(AbstractTasksModel::IsLauncher).toBool()) {
                        insertPos = i + 1;
                    } else {
                        break;
                    }
                }

                sortedPreFilterRows.move(row, insertPos);
            // Anything else goes after its right-most app sibling, if any. If there are
            // none it just stays put.
            } else {
                for (int i = (row - 1); i >= 0; --i) {
                    const QModelIndex &concatProxyIndex = concatProxyModel->index(sortedPreFilterRows.at(i), 0);

                    if (appsMatch(concatProxyIndex, idx)) {
                        sortedPreFilterRows.move(row, i + 1);

                        break;
                    }
                }
            }
        }
    }
}

void TasksModel::Private::syncManualSortMapForGroup(const QModelIndex &parent)
{
    const int childCount = q->rowCount(parent);

    if (childCount != -1) {
        const QModelIndex &preFilterParent = preFilterIndex(q->mapToSource(parent));

        // We're moving the trailing children to the sort map position of
        // the first child, so we're skipping the first child.
        for (int i = 1; i < childCount; ++i) {
            const QModelIndex &preFilterChildIndex = preFilterIndex(q->mapToSource(parent.child(i, 0)));
            const int childSortIndex = sortedPreFilterRows.indexOf(preFilterChildIndex.row());
            const int parentSortIndex = sortedPreFilterRows.indexOf(preFilterParent.row());
            const int insertPos = (parentSortIndex + i) + ((parentSortIndex + i) > childSortIndex ? -1 : 0);
            sortedPreFilterRows.move(childSortIndex, insertPos);
        }
    }
}

QModelIndex TasksModel::Private::preFilterIndex(const QModelIndex &sourceIndex) const {
    return filterProxyModel->mapToSource(groupingProxyModel->mapToSource(sourceIndex));
}

void TasksModel::Private::updateActivityTaskCounts()
{
    // Collects the number of window tasks on each activity.

    activityTaskCounts.clear();

    if (!windowTasksModel || !activityInfo) {
        return;
    }

    foreach(const QString &activity, activityInfo->runningActivities()) {
        activityTaskCounts.insert(activity, 0);
    }

    for (int i = 0; i < windowTasksModel->rowCount(); ++i) {
        const QModelIndex &windowIndex = windowTasksModel->index(i, 0);
        const QStringList &activities = windowIndex.data(AbstractTasksModel::Activities).toStringList();

        if (activities.isEmpty()) {
            QMutableHashIterator<QString, int> i(activityTaskCounts);

            while (i.hasNext()) {
                i.next();
                i.setValue(i.value() + 1);
            }
        } else {
            foreach(const QString &activity, activities) {
                ++activityTaskCounts[activity];
            }
        }
    }
}

void TasksModel::Private::forceResort()
{
    // HACK: This causes QSortFilterProxyModel to run all rows through
    // our lessThan() implementation again.
    q->setDynamicSortFilter(false);
    q->setDynamicSortFilter(true);
}

bool TasksModel::Private::lessThan(const QModelIndex &left, const QModelIndex &right, bool sortOnlyLaunchers) const
{
    // Launcher tasks go first.
    // When launchInPlace is enabled, startup and window tasks are sorted
    // as the launchers they replace (see also move()).
    if (left.data(AbstractTasksModel::IsLauncher).toBool() && right.data(AbstractTasksModel::IsLauncher).toBool()) {
        return (left.row() < right.row());
    } else if (left.data(AbstractTasksModel::IsLauncher).toBool() && !right.data(AbstractTasksModel::IsLauncher).toBool()) {
        if (launchInPlace) {
            const int rightPos = q->launcherPosition(right.data(AbstractTasksModel::LauncherUrl).toUrl());

            if (rightPos != -1) {
                return (left.row() < rightPos);
            }
        }

        return true;
    } else if (!left.data(AbstractTasksModel::IsLauncher).toBool() && right.data(AbstractTasksModel::IsLauncher).toBool()) {
        if (launchInPlace) {
            const int leftPos = q->launcherPosition(left.data(AbstractTasksModel::LauncherUrl).toUrl());

            if (leftPos != -1) {
                return (leftPos < right.row());
            }
        }

        return false;
    } else if (launchInPlace) {
        const int leftPos = q->launcherPosition(left.data(AbstractTasksModel::LauncherUrl).toUrl());
        const int rightPos = q->launcherPosition(right.data(AbstractTasksModel::LauncherUrl).toUrl());

        if (leftPos != -1 && rightPos != -1) {
            return (leftPos < rightPos);
        } else if (leftPos != -1 && rightPos == -1) {
            return true;
        } else if (leftPos == -1 && rightPos != -1) {
            return false;
        }
    }

    // If told to stop after launchers we fall through to the existing map if it exists.
    if (sortOnlyLaunchers && !sortedPreFilterRows.isEmpty()) {
        return (sortedPreFilterRows.indexOf(left.row()) < sortedPreFilterRows.indexOf(right.row()));
    }

    // Sort other cases by sort mode.
    switch (sortMode) {
        case SortVirtualDesktop: {
            const QVariant &leftDesktopVariant = left.data(AbstractTasksModel::VirtualDesktop);
            bool leftOk = false;
            const int leftDesktop = leftDesktopVariant.toInt(&leftOk);

            const QVariant &rightDesktopVariant = right.data(AbstractTasksModel::VirtualDesktop);
            bool rightOk = false;
            const int rightDesktop = rightDesktopVariant.toInt(&rightOk);

            if (leftOk && rightOk && (leftDesktop != rightDesktop)) {
                return (leftDesktop < rightDesktop);
            } else if (leftOk && !rightOk) {
                return false;
            } else if (!leftOk && rightOk) {
                return true;
            }
        }
        case SortActivity: {
            // updateActivityTaskCounts() counts the number of window tasks on each
            // activity. This will sort tasks by comparing a cumulative score made
            // up of the task counts for each acvtivity a task is assigned to, and
            // otherwise fall through to alphabetical sorting.
            int leftScore = -1;
            int rightScore = -1;

            const QStringList &leftActivities = left.data(AbstractTasksModel::Activities).toStringList();

            if (!leftActivities.isEmpty()) {
                foreach(const QString& activity, leftActivities) {
                    leftScore += activityTaskCounts[activity];
                }
            }

            const QStringList &rightActivities = right.data(AbstractTasksModel::Activities).toStringList();

            if (!rightActivities.isEmpty()) {
                foreach(const QString& activity, rightActivities) {
                    rightScore += activityTaskCounts[activity];
                }
            }

            if (leftScore == -1 || rightScore == -1) {
                const QList<int> &counts = activityTaskCounts.values();
                const int sumScore = std::accumulate(counts.begin(), counts.end(), 0);

                if (leftScore == -1) {
                    leftScore = sumScore;
                }

                if (rightScore == -1) {
                    rightScore = sumScore;
                }
            }

            if (leftScore != rightScore) {
                return (leftScore > rightScore);
            }
        }
        // Fall through to source order if sorting is disabled or manual, or alphabetical by app name otherwise.
        default: {
            if (sortMode == SortDisabled) {
                return (left.row() < right.row());
            } else {
                const QString &leftSortString = left.data(AbstractTasksModel::AppName).toString()
                    + left.data(Qt::DisplayRole).toString();

                const QString &rightSortString = right.data(AbstractTasksModel::AppName).toString()
                    + right.data(Qt::DisplayRole).toString();

                return (leftSortString.localeAwareCompare(rightSortString) < 0);
            }
        }
    }
}

TasksModel::TasksModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d(new Private(this))
{
    d->initModels();

    // Start sorting.
    sort(0);
}

TasksModel::~TasksModel()
{
}

QHash<int, QByteArray> TasksModel::roleNames() const
{
    if (d->windowTasksModel) {
        return d->windowTasksModel->roleNames();
    }

    return QHash<int, QByteArray>();
}

int TasksModel::rowCount(const QModelIndex &parent) const
{
    return QSortFilterProxyModel::rowCount(parent);
}

int TasksModel::launcherCount() const
{
    // TODO: Optimize algorithm or cache the output.

    QList<QUrl> launchers = QUrl::fromStringList(d->launcherTasksModel->launcherList());

    for(int i = 0; i < d->filterProxyModel->rowCount(); ++i) {
        const QModelIndex &filterIndex = d->filterProxyModel->index(i, 0);

        if (!filterIndex.data(AbstractTasksModel::IsLauncher).toBool()) {
            const QUrl &launcherUrl = filterIndex.data(AbstractTasksModel::LauncherUrl).toUrl();

            QMutableListIterator<QUrl> it(launchers);

            while(it.hasNext()) {
                it.next();

                if (launcherUrlsMatch(launcherUrl, it.value(), IgnoreQueryItems)) {
                    it.remove();
                }
            }
        }
    }

    return launchers.count();
}

bool TasksModel::anyTaskDemandsAttention() const
{
    return d->anyTaskDemandsAttention;
}

int TasksModel::virtualDesktop() const
{
    return d->filterProxyModel->virtualDesktop();
}

void TasksModel::setVirtualDesktop(int virtualDesktop)
{
    d->filterProxyModel->setVirtualDesktop(virtualDesktop);
}

int TasksModel::screen() const
{
    return d->filterProxyModel->screen();
}

void TasksModel::setScreen(int screen)
{
    d->filterProxyModel->setScreen(screen);
}

QString TasksModel::activity() const
{
    return d->filterProxyModel->activity();
}

void TasksModel::setActivity(const QString &activity)
{
    d->filterProxyModel->setActivity(activity);
}

bool TasksModel::filterByVirtualDesktop() const
{
    return d->filterProxyModel->filterByVirtualDesktop();
}

void TasksModel::setFilterByVirtualDesktop(bool filter)
{
    d->filterProxyModel->setFilterByVirtualDesktop(filter);
}

bool TasksModel::filterByScreen() const
{
    return d->filterProxyModel->filterByScreen();
}

void TasksModel::setFilterByScreen(bool filter)
{
    d->filterProxyModel->setFilterByScreen(filter);
}

bool TasksModel::filterByActivity() const
{
    return d->filterProxyModel->filterByActivity();
}

void TasksModel::setFilterByActivity(bool filter)
{
    d->filterProxyModel->setFilterByActivity(filter);
}

bool TasksModel::filterNotMinimized() const
{
    return d->filterProxyModel->filterNotMinimized();
}

void TasksModel::setFilterNotMinimized(bool filter)
{
    d->filterProxyModel->setFilterNotMinimized(filter);
}

TasksModel::SortMode TasksModel::sortMode() const
{
    return d->sortMode;
}

void TasksModel::setSortMode(SortMode mode)
{
    if (d->sortMode != mode) {
        if (mode == SortManual) {
            d->updateManualSortMap();
        } else if (d->sortMode == SortManual) {
            d->sortedPreFilterRows.clear();
        }

        if (mode == SortActivity) {
            if (!d->activityInfo) {
                d->activityInfo = new ActivityInfo();
            }

            ++d->activityInfoUsers;

            d->updateActivityTaskCounts();
            setSortRole(AbstractTasksModel::Activities);
        } else if (d->sortMode == SortActivity) {
            --d->activityInfoUsers;

            if (!d->activityInfoUsers) {
                delete d->activityInfo;
                d->activityInfo = nullptr;
            }

            d->activityTaskCounts.clear();
            setSortRole(Qt::DisplayRole);
        }

        d->sortMode = mode;

        d->forceResort();

        emit sortModeChanged();
    }
}

bool TasksModel::separateLaunchers() const
{
    return d->separateLaunchers;
}

void TasksModel::setSeparateLaunchers(bool separate)
{
    return; // FIXME TODO: Disable until done.

    if (d->separateLaunchers != separate) {
        d->separateLaunchers = separate;

        d->forceResort();

        emit separateLaunchersChanged();
    }
}

bool TasksModel::launchInPlace() const
{
    return d->launchInPlace;
}

void TasksModel::setLaunchInPlace(bool launchInPlace)
{
    if (d->launchInPlace != launchInPlace) {
        d->launchInPlace = launchInPlace;

        d->forceResort();

        emit launchInPlaceChanged();
    }
}

TasksModel::GroupMode TasksModel::groupMode() const
{
    if (!d->groupingProxyModel) {
        return GroupDisabled;
    }

    return d->groupingProxyModel->groupMode();
}

void TasksModel::setGroupMode(GroupMode mode)
{
    if (d->groupingProxyModel) {
        d->groupingProxyModel->setGroupMode(mode);
    }
}

int TasksModel::groupingWindowTasksThreshold() const
{
    if (!d->groupingProxyModel) {
        return -1;
    }

    return d->groupingProxyModel->windowTasksThreshold();
}

void TasksModel::setGroupingWindowTasksThreshold(int threshold)
{
    if (d->groupingProxyModel) {
        d->groupingProxyModel->setWindowTasksThreshold(threshold);
    }
}

QStringList TasksModel::groupingAppIdBlacklist() const
{
    if (!d->groupingProxyModel) {
        return QStringList();
    }

    return d->groupingProxyModel->blacklistedAppIds();
}

void TasksModel::setGroupingAppIdBlacklist(const QStringList &list)
{
    if (d->groupingProxyModel) {
        d->groupingProxyModel->setBlacklistedAppIds(list);
    }
}

QStringList TasksModel::groupingLauncherUrlBlacklist() const
{
    if (!d->groupingProxyModel) {
        return QStringList();
    }

    return d->groupingProxyModel->blacklistedLauncherUrls();
}

void TasksModel::setGroupingLauncherUrlBlacklist(const QStringList &list)
{
    if (d->groupingProxyModel) {
        d->groupingProxyModel->setBlacklistedLauncherUrls(list);
    }
}

QStringList TasksModel::launcherList() const
{
    if (d->launcherTasksModel) {
        return d->launcherTasksModel->launcherList();
    }

    return QStringList();
}

void TasksModel::setLauncherList(const QStringList &launchers)
{
    if (d->launcherTasksModel) {
        d->launcherTasksModel->setLauncherList(launchers);
    }
}

bool TasksModel::requestAddLauncher(const QUrl &url)
{
    if (d->launcherTasksModel) {
        bool added = d->launcherTasksModel->requestAddLauncher(url);

        // If using manual sorting and launch-in-place sorting, we need
        // to trigger a sort map update to move any window tasks to their
        // launcher position now.
        if (added && d->sortMode == SortManual && (d->launchInPlace || !d->separateLaunchers)) {
            d->updateManualSortMap();
            d->forceResort();
        }

        return added;
    }

    return false;
}

bool TasksModel::requestRemoveLauncher(const QUrl &url)
{
    if (d->launcherTasksModel) {
        return d->launcherTasksModel->requestRemoveLauncher(url);
    }

    return false;
}

int TasksModel::launcherPosition(const QUrl &url) const
{
    if (d->launcherTasksModel) {
        return d->launcherTasksModel->launcherPosition(url);
    }

    return -1;
}

void TasksModel::requestActivate(const QModelIndex &index)
{
    if (index.isValid() && index.model() == this) {
        d->groupingProxyModel->requestActivate(mapToSource(index));
    }
}

void TasksModel::requestNewInstance(const QModelIndex &index)
{
    if (index.isValid() && index.model() == this) {
        d->groupingProxyModel->requestNewInstance(mapToSource(index));
    }
}

void TasksModel::requestClose(const QModelIndex &index)
{
    if (index.isValid() && index.model() == this) {
        d->groupingProxyModel->requestClose(mapToSource(index));
    }
}

void TasksModel::requestMove(const QModelIndex &index)
{
    if (index.isValid() && index.model() == this) {
        d->groupingProxyModel->requestMove(mapToSource(index));
    }
}

void TasksModel::requestResize(const QModelIndex &index)
{
    if (index.isValid() && index.model() == this) {
        d->groupingProxyModel->requestResize(mapToSource(index));
    }
}

void TasksModel::requestToggleMinimized(const QModelIndex &index)
{
    if (index.isValid() && index.model() == this) {
        d->groupingProxyModel->requestToggleMinimized(mapToSource(index));
    }
}

void TasksModel::requestToggleMaximized(const QModelIndex &index)
{
    if (index.isValid() && index.model() == this) {
        d->groupingProxyModel->requestToggleMaximized(mapToSource(index));
    }
}

void TasksModel::requestToggleKeepAbove(const QModelIndex &index)
{
    if (index.isValid() && index.model() == this) {
        d->groupingProxyModel->requestToggleKeepAbove(mapToSource(index));
    }
}

void TasksModel::requestToggleKeepBelow(const QModelIndex &index)
{
    if (index.isValid() && index.model() == this) {
        d->groupingProxyModel->requestToggleKeepBelow(mapToSource(index));
    }
}

void TasksModel::requestToggleFullScreen(const QModelIndex &index)
{
    if (index.isValid() && index.model() == this) {
        d->groupingProxyModel->requestToggleFullScreen(mapToSource(index));
    }
}

void TasksModel::requestToggleShaded(const QModelIndex &index)
{
    if (index.isValid() && index.model() == this) {
        d->groupingProxyModel->requestToggleShaded(mapToSource(index));
    }
}

void TasksModel::requestVirtualDesktop(const QModelIndex &index, qint32 desktop)
{
    if (index.isValid() && index.model() == this) {
        d->groupingProxyModel->requestVirtualDesktop(mapToSource(index), desktop);
    }
}

void TasksModel::requestPublishDelegateGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate)
{
    if (index.isValid() && index.model() == this) {
        d->groupingProxyModel->requestPublishDelegateGeometry(mapToSource(index), geometry, delegate);
    }
}

void TasksModel::requestToggleGrouping(const QModelIndex &index)
{
    if (index.isValid() && index.model() == this) {
        d->groupingProxyModel->requestToggleGrouping(mapToSource(index));
    }
}

bool TasksModel::move(int row, int newPos)
{
    if (d->sortMode != SortManual || row == newPos || newPos < 0 || newPos >= rowCount()) {
        return false;
    }

    const QModelIndex &idx = index(row, 0);
    bool isLauncherMove = false;

    // Figure out if we're moving a launcher so we can run barrier checks.
    if (idx.isValid()) {
        if (idx.data(AbstractTasksModel::IsLauncher).toBool()) {
            isLauncherMove = true;
        // When using launch-in-place sorting, launcher-backed window tasks act as launchers.
        } else if ((d->launchInPlace || !d->separateLaunchers)
            && idx.data(AbstractTasksModel::IsWindow).toBool()) {
            const QUrl &launcherUrl = idx.data(AbstractTasksModel::LauncherUrl).toUrl();
            const int launcherPos = launcherPosition(launcherUrl);

            if (launcherPos != -1) {
                isLauncherMove = true;
            }
        }
    } else {
        return false;
    }

    if (d->separateLaunchers) {
        const int firstTask = (d->launchInPlace ? d->launcherTasksModel->rowCount() : launcherCount());

        // Don't allow launchers to be moved past the last launcher.
        if (isLauncherMove && newPos >= firstTask) {
            return false;
        }

        // Don't allow tasks to be moved into the launchers.
        if (!isLauncherMove && newPos < firstTask) {
            return false;
        }
    }

    beginMoveRows(QModelIndex(), row, row, QModelIndex(), (newPos >row) ? newPos + 1 : newPos);

    // Translate to sort map indices.
    const QModelIndex &rowIndex = index(row, 0);
    const QModelIndex &preFilterRowIndex = d->preFilterIndex(mapToSource(rowIndex));
    row = d->sortedPreFilterRows.indexOf(preFilterRowIndex.row());
    newPos = d->sortedPreFilterRows.indexOf(d->preFilterIndex(mapToSource(index(newPos, 0))).row());

    // Update sort mapping.
    d->sortedPreFilterRows.move(row, newPos);

    endMoveRows();

    // Move children along with the group.
    // This can be safely done after the row move transaction as the sort
    // map isn't consulted for rows below the top level.
    if (groupMode() != GroupDisabled && rowCount(rowIndex)) {
        d->syncManualSortMapForGroup(rowIndex);
    }

    // Resort.
    d->forceResort();

    // Setup for syncLaunchers().
    d->launcherSortingDirty = isLauncherMove;

    return true;
}

void TasksModel::syncLaunchers()
{
    // Writes the launcher order exposed through the model back to the launcher
    // tasks model, committing any move() operations to persistent state.

    if (!d->launcherTasksModel || !d->launcherSortingDirty) {
        return;
    }

    QMap<int, QUrl> sortedLaunchers;

    foreach(const QUrl &launcherUrl, launcherList()) {
        int row = -1;

        for (int i = 0; i < rowCount(); ++i) {
            const QUrl &rowLauncherUrl = index(i, 0).data(AbstractTasksModel::LauncherUrl).toUrl();

            if (launcherUrlsMatch(launcherUrl, rowLauncherUrl, IgnoreQueryItems)) {
                row = i;
                break;
            }
        }

        if (row != -1) {
            sortedLaunchers.insert(row, launcherUrl);
        }
    }

    setLauncherList(QUrl::toStringList(sortedLaunchers.values()));
    d->launcherSortingDirty = false;
}

QModelIndex TasksModel::activeTask() const
{
    for (int i = 0; i < rowCount(); ++i) {
        const QModelIndex &idx = index(i, 0);

        if (idx.data(AbstractTasksModel::IsActive).toBool()) {
            if (groupMode() != GroupDisabled && rowCount(idx)) {
                for (int j = 0; j < rowCount(idx); ++j) {
                    const QModelIndex &child = idx.child(j, 0);

                    if (child.data(AbstractTasksModel::IsActive).toBool()) {
                        return child;
                    }
                }
            } else {
                return idx;
            }
        }
    }

    return QModelIndex();
}

QModelIndex TasksModel::makeModelIndex(int row, int childRow) const
{
    if (row < 0 || row >= rowCount()) {
        return QModelIndex();
    }

    const QModelIndex &parent = index(row, 0);

    if (childRow == -1) {
        return index(row, 0);
    } else {
        const QModelIndex &parent = index(row, 0);

        if (childRow < rowCount(parent)) {
            return parent.child(childRow, 0);
        }
    }

    return QModelIndex();
}

bool TasksModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    // All our filtering occurs at the top-level; group children always go through.
    if (sourceParent.isValid()) {
        return true;
    }

    const QModelIndex &sourceIndex = sourceModel()->index(sourceRow, 0);
    const QString &appId = sourceIndex.data(AbstractTasksModel::AppId).toString();
    const QString &appName = sourceIndex.data(AbstractTasksModel::AppName).toString();

    // Filter startup tasks we already have a window task for.
    if (sourceIndex.data(AbstractTasksModel::IsStartup).toBool()) {
        for (int i = 0; i < d->windowTasksModel->rowCount(); ++i) {
            const QModelIndex &windowIndex = d->windowTasksModel->index(i, 0);

            if (appId == windowIndex.data(AbstractTasksModel::AppId).toString()
                || appName == windowIndex.data(AbstractTasksModel::AppName).toString()) {
                return false;
            }
        }
    }

    // Filter launcher tasks we already have a startup or window task for (that
    // got through filtering).
    if (sourceIndex.data(AbstractTasksModel::IsLauncher).toBool()) {
        const QUrl &launcherUrl = sourceIndex.data(AbstractTasksModel::LauncherUrl).toUrl();

        for (int i = 0; i < d->filterProxyModel->rowCount(); ++i) {
            const QModelIndex &filteredIndex = d->filterProxyModel->index(i, 0);

            if (!filteredIndex.data(AbstractTasksModel::IsWindow).toBool() &&
                !filteredIndex.data(AbstractTasksModel::IsStartup).toBool()) {
                continue;
            }

            const QString &filteredAppId = filteredIndex.data(AbstractTasksModel::AppId).toString();

            if ((!appId.isEmpty() && appId == filteredAppId)
                || launcherUrlsMatch(launcherUrl, filteredIndex.data(AbstractTasksModel::LauncherUrl).toUrl(),
                IgnoreQueryItems)) {
                emit launcherCountChanged();

                return false;
            }
        }
    }

    return true;
}

bool TasksModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    // In manual sort mode we sort top-level items by referring to a map we keep.
    // Insertions into the map are placed using a combination of Private::lessThan
    // and simple append behavior. Child items are sorted alphabetically.
    if (d->sortMode == SortManual && !left.parent().isValid() && !right.parent().isValid()) {
        return (d->sortedPreFilterRows.indexOf(d->preFilterIndex(left).row())
            < d->sortedPreFilterRows.indexOf(d->preFilterIndex(right).row()));
    }

    return d->lessThan(left, right);
}

}
