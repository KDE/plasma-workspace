/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "taskfilterproxymodel.h"
#include "abstracttasksmodel.h"

#include "launchertasksmodel_p.h"

#include "config-X11.h"
#if HAVE_X11
#include <QGuiApplication>
#include <QScreen>

#include <KWindowSystem>
#endif

namespace TaskManager
{
class Q_DECL_HIDDEN TaskFilterProxyModel::Private
{
public:
    Private(TaskFilterProxyModel *q);

    AbstractTasksModelIface *sourceTasksModel = nullptr;

    QVariant virtualDesktop;
    QRect screenGeometry;
    QRect regionGeometry;
    QString activity;

    bool filterByVirtualDesktop = false;
    bool filterByScreen = false;
    bool filterByActivity = false;
    RegionFilterMode::Mode filterByRegion = RegionFilterMode::Mode::Disabled;
    bool filterMinimized = false;
    bool filterNotMinimized = false;
    bool filterNotMaximized = false;
    bool filterHidden = false;
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

        Q_EMIT virtualDesktopChanged();
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

        Q_EMIT screenGeometryChanged();
    }
}

QRect TaskFilterProxyModel::regionGeometry() const
{
    return d->regionGeometry;
}

void TaskFilterProxyModel::setRegionGeometry(const QRect &geometry)
{
    if (d->regionGeometry == geometry) {
        return;
    }
    d->regionGeometry = geometry;

    if (d->filterByRegion != RegionFilterMode::Mode::Disabled) {
        invalidateFilter();
    }

    Q_EMIT regionGeometryChanged();
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

        Q_EMIT activityChanged();
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

        Q_EMIT filterByVirtualDesktopChanged();
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

        Q_EMIT filterByScreenChanged();
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

        Q_EMIT filterByActivityChanged();
    }
}

RegionFilterMode::Mode TaskFilterProxyModel::filterByRegion() const
{
    return d->filterByRegion;
}

void TaskFilterProxyModel::setFilterByRegion(RegionFilterMode::Mode mode)
{
    if (d->filterByRegion == mode) {
        return;
    }

    d->filterByRegion = mode;
    invalidateFilter();
    Q_EMIT filterByActivityChanged();
}

bool TaskFilterProxyModel::filterMinimized() const
{
    return d->filterMinimized;
}

void TaskFilterProxyModel::setFilterMinimized(bool filter)
{
    if (d->filterMinimized == filter) {
        return;
    }

    d->filterMinimized = filter;
    invalidateFilter();

    Q_EMIT filterMinimizedChanged();
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

        Q_EMIT filterNotMinimizedChanged();
    }
}

bool TaskFilterProxyModel::filterNotMaximized() const
{
    return d->filterNotMaximized;
}

void TaskFilterProxyModel::setFilterNotMaximized(bool filter)
{
    if (d->filterNotMaximized != filter) {
        d->filterNotMaximized = filter;

        invalidateFilter();

        Q_EMIT filterNotMaximizedChanged();
    }
}

bool TaskFilterProxyModel::filterHidden() const
{
    return d->filterHidden;
}

void TaskFilterProxyModel::setFilterHidden(bool filter)
{
    if (d->filterHidden != filter) {
        d->filterHidden = filter;

        invalidateFilter();

        Q_EMIT filterHiddenChanged();
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

        Q_EMIT filterSkipTaskbarChanged();
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

        Q_EMIT filterSkipPagerChanged();
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

        Q_EMIT demandingAttentionSkipsFiltersChanged();
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

    // Filter by region
    if (d->filterByRegion != RegionFilterMode::Mode::Disabled && d->regionGeometry.isValid()) {
        QRect windowGeometry = sourceIdx.data(AbstractTasksModel::Geometry).toRect();

        QRect regionGeometry = d->regionGeometry;
#if HAVE_X11
        if (static const bool isX11 = KWindowSystem::isPlatformX11(); isX11 && windowGeometry.isValid()) {
            // On X11, in regionGeometry, the original point of the topLeft position belongs to the device coordinate system
            // but the size belongs to the logical coordinate system (which means the reported size is already divided by DPR)
            // Converting regionGeometry to device coordinate system is better than converting windowGeometry to logical
            // coordinate system because the window may span multiple screens, while the region is always on one screen.
            const double devicePixelRatio = qGuiApp->devicePixelRatio();
            const QPoint screenTopLeft = d->screenGeometry.topLeft();
            const QPoint regionTopLeft =
                screenTopLeft + QPoint(regionGeometry.x() - screenTopLeft.x(), regionGeometry.y() - screenTopLeft.y()) * devicePixelRatio;
            regionGeometry = QRect(regionTopLeft, regionGeometry.size() * devicePixelRatio);
        }
#endif
        switch (d->filterByRegion) {
        case RegionFilterMode::Mode::Inside: {
            if (!regionGeometry.contains(windowGeometry)) {
                return false;
            }
            break;
        }
        case RegionFilterMode::Mode::Intersect: {
            if (!regionGeometry.intersects(windowGeometry)) {
                return false;
            }
            break;
        }
        case RegionFilterMode::Mode::Outside: {
            if (regionGeometry.contains(windowGeometry)) {
                return false;
            }
            break;
        }
        default:
            break;
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

    // Filter out minimized windows
    if (d->filterMinimized) {
        const bool isMinimized = sourceIdx.data(AbstractTasksModel::IsMinimized).toBool();

        if (isMinimized) {
            return false;
        }
    }

    // Filter not maximized.
    if (d->filterNotMaximized) {
        bool isMaximized = sourceIdx.data(AbstractTasksModel::IsMaximized).toBool();

        if (!isMaximized) {
            return false;
        }
    }

    // Filter hidden.
    if (d->filterHidden) {
        bool isHidden = sourceIdx.data(AbstractTasksModel::IsHidden).toBool();

        if (isHidden) {
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
