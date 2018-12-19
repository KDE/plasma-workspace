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

#include "taskfilterproxymodel.h"
#include "abstracttasksmodel.h"

#include "launchertasksmodel_p.h"

namespace TaskManager
{

class Q_DECL_HIDDEN TaskFilterProxyModel::Private
{
public:
    Private(TaskFilterProxyModel *q);

    AbstractTasksModelIface *sourceTasksModel = nullptr;

    QVariant virtualDesktop;
    QRect screenGeometry;
    QString activity;

    bool filterByVirtualDesktop = false;
    bool filterByScreen = false;
    bool filterByActivity = false;
    bool filterNotMinimized = false;
    bool filterSkipTaskbar = true;
    bool filterSkipPager = false;

    bool demandingAttentionSkipsFilters = true;
};

TaskFilterProxyModel::Private::Private(TaskFilterProxyModel *)
{
}

TaskFilterProxyModel::TaskFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d(new Private(this))
{
}

TaskFilterProxyModel::~TaskFilterProxyModel()
{
}

void TaskFilterProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    d->sourceTasksModel = dynamic_cast<AbstractTasksModelIface *>(sourceModel);

    QSortFilterProxyModel::setSourceModel(sourceModel);
}

QVariant TaskFilterProxyModel::virtualDesktop() const
{
    return d->virtualDesktop;
}

void TaskFilterProxyModel::setVirtualDesktop(const QVariant &desktop)
{
    if (d->virtualDesktop != desktop) {
        d->virtualDesktop = desktop;

        if (d->filterByVirtualDesktop) {
            invalidateFilter();
        }

        emit virtualDesktopChanged();
    }
}

QRect TaskFilterProxyModel::screenGeometry() const
{
    return d->screenGeometry;
}

void TaskFilterProxyModel::setScreenGeometry(const QRect &geometry)
{
    if (d->screenGeometry != geometry) {
        d->screenGeometry = geometry;

        if (d->filterByScreen) {
            invalidateFilter();
        }

        emit screenGeometryChanged();
    }
}

QString TaskFilterProxyModel::activity() const
{
    return d->activity;
}

void TaskFilterProxyModel::setActivity(const QString &activity)
{
    if (d->activity != activity) {
        d->activity = activity;

        if (d->filterByActivity) {
            invalidateFilter();
        }

        emit activityChanged();
    }
}

bool TaskFilterProxyModel::filterByVirtualDesktop() const
{
    return d->filterByVirtualDesktop;
}

void TaskFilterProxyModel::setFilterByVirtualDesktop(bool filter)
{
    if (d->filterByVirtualDesktop != filter) {
        d->filterByVirtualDesktop = filter;

        invalidateFilter();

        emit filterByVirtualDesktopChanged();
    }
}

bool TaskFilterProxyModel::filterByScreen() const
{
    return d->filterByScreen;
}

void TaskFilterProxyModel::setFilterByScreen(bool filter)
{
    if (d->filterByScreen != filter) {
        d->filterByScreen = filter;

        invalidateFilter();

        emit filterByScreenChanged();
    }
}

bool TaskFilterProxyModel::filterByActivity() const
{
    return d->filterByActivity;
}

void TaskFilterProxyModel::setFilterByActivity(bool filter)
{
    if (d->filterByActivity != filter) {
        d->filterByActivity = filter;

        invalidateFilter();

        emit filterByActivityChanged();
    }
}

bool TaskFilterProxyModel::filterNotMinimized() const
{
    return d->filterNotMinimized;
}

void TaskFilterProxyModel::setFilterNotMinimized(bool filter)
{
    if (d->filterNotMinimized != filter) {
        d->filterNotMinimized = filter;

        invalidateFilter();

        emit filterNotMinimizedChanged();
    }
}

bool TaskFilterProxyModel::filterSkipTaskbar() const
{
    return d->filterSkipTaskbar;
}

void TaskFilterProxyModel::setFilterSkipTaskbar(bool filter)
{
    if (d->filterSkipTaskbar != filter) {
        d->filterSkipTaskbar = filter;

        invalidateFilter();

        emit filterSkipTaskbarChanged();
    }
}

bool TaskFilterProxyModel::filterSkipPager() const
{
    return d->filterSkipPager;
}

void TaskFilterProxyModel::setFilterSkipPager(bool filter)
{
    if (d->filterSkipPager != filter) {
        d->filterSkipPager = filter;

        invalidateFilter();

        emit filterSkipPagerChanged();
    }
}

bool TaskFilterProxyModel::demandingAttentionSkipsFilters() const
{
    return d->demandingAttentionSkipsFilters;
}

void TaskFilterProxyModel::setDemandingAttentionSkipsFilters(bool skip)
{
    if (d->demandingAttentionSkipsFilters != skip) {
        d->demandingAttentionSkipsFilters = skip;

        invalidateFilter();

        emit demandingAttentionSkipsFiltersChanged();
    }
}

QModelIndex TaskFilterProxyModel::mapIfaceToSource(const QModelIndex &index) const
{
    return mapToSource(index);
}

bool TaskFilterProxyModel::acceptsRow(int sourceRow) const
{
    const QModelIndex &sourceIdx = sourceModel()->index(sourceRow, 0);

    if (!sourceIdx.isValid()) {
        return false;
    }

    // Filter tasks that are not to be shown on the task bar.
    if (d->filterSkipTaskbar && sourceIdx.data(AbstractTasksModel::SkipTaskbar).toBool()) {
        return false;
    }

    // Filter tasks that are not to be shown on the pager.
    if (d->filterSkipPager && sourceIdx.data(AbstractTasksModel::SkipPager).toBool()) {
        return false;
    }

    // Filter by virtual desktop.
    if (d->filterByVirtualDesktop && !d->virtualDesktop.isNull()) {
        if (!sourceIdx.data(AbstractTasksModel::IsOnAllVirtualDesktops).toBool()
            && (!d->demandingAttentionSkipsFilters || !sourceIdx.data(AbstractTasksModel::IsDemandingAttention).toBool())) {
            const QVariantList &virtualDesktops = sourceIdx.data(AbstractTasksModel::VirtualDesktops).toList();

            if (!virtualDesktops.isEmpty() && !virtualDesktops.contains(d->virtualDesktop)) {
                return false;
            }
        }
    }

    // Filter by screen.
    if (d->filterByScreen && d->screenGeometry.isValid()) {
        const QRect &screenGeometry = sourceIdx.data(AbstractTasksModel::ScreenGeometry).toRect();

        if (screenGeometry.isValid() && screenGeometry != d->screenGeometry) {
            return false;
        }
    }

    // Filter by activity.
    if (d->filterByActivity && !d->activity.isEmpty()) {
        if (!d->demandingAttentionSkipsFilters || !sourceIdx.data(AbstractTasksModel::IsDemandingAttention).toBool()) {
            const QVariant &activities = sourceIdx.data(AbstractTasksModel::Activities);

            if (!activities.isNull()) {
                const QStringList l = activities.toStringList();

                if (!l.isEmpty() && !l.contains(NULL_UUID) && !l.contains(d->activity)) {
                    return false;
                }
            }
        }
    }

    // Filter not minimized.
    if (d->filterNotMinimized) {
        bool isMinimized = sourceIdx.data(AbstractTasksModel::IsMinimized).toBool();

        if (!isMinimized) {
            return false;
        }
    }

    return true;
}

bool TaskFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent)

    return acceptsRow(sourceRow);
}

}
