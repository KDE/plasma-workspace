/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "maximizedwindowmonitor.h"

#include <QGuiApplication>
#include <QScreen>

#include "abstracttasksmodel.h" // For enums
#include "activityinfo.h"
#include "virtualdesktopinfo.h"

#include "debug.h"

class MaximizedWindowMonitor::Private
{
public:
    explicit Private(MaximizedWindowMonitor *q);

    void init();

private:
    Q_DISABLE_COPY(Private);

    MaximizedWindowMonitor *q = nullptr;

    bool m_ready = false;

    TaskManager::ActivityInfo *m_activityInfo = nullptr;
    TaskManager::VirtualDesktopInfo *m_virtualDesktopInfo = nullptr;

    QRect m_geometry;

    friend class MaximizedWindowMonitor;
};

MaximizedWindowMonitor::Private::Private(MaximizedWindowMonitor *q)
    : q(q)
    , m_activityInfo(new TaskManager::ActivityInfo(q))
    , m_virtualDesktopInfo(new TaskManager::VirtualDesktopInfo(q))
{
}

void MaximizedWindowMonitor::Private::init()
{
    q->setSortMode(SortMode::SortActivity);
    q->setGroupMode(GroupMode::GroupDisabled);

    auto currentActivity = std::bind(&TaskManager::ActivityInfo::currentActivity, m_activityInfo);
    auto setCurrentActivity = std::bind(&TaskManager::TasksModel::setActivity, q, currentActivity);
    setCurrentActivity();
    q->connect(m_activityInfo, &TaskManager::ActivityInfo::currentActivityChanged, q, setCurrentActivity);

    auto currentDesktop = std::bind(&TaskManager::VirtualDesktopInfo::currentDesktop, m_virtualDesktopInfo);
    auto setCurrentDesktop = std::bind(&TaskManager::TasksModel::setVirtualDesktop, q, currentDesktop);
    setCurrentDesktop();
    q->connect(m_virtualDesktopInfo, &TaskManager::VirtualDesktopInfo::currentDesktopChanged, q, setCurrentDesktop);

    q->setFilterMinimized(true);
    q->setFilterByActivity(true);
    q->setFilterByVirtualDesktop(true);
}

MaximizedWindowMonitor::MaximizedWindowMonitor(QObject *parent)
    : TaskManager::TasksModel(parent)
    , d(std::make_unique<Private>(this))
{
    d->init();
}

MaximizedWindowMonitor::~MaximizedWindowMonitor()
{
}

QRect MaximizedWindowMonitor::targetRect() const
{
    return d->m_geometry;
}

void MaximizedWindowMonitor::setTargetRect(const QRect &rect)
{
    if (d->m_geometry == rect) {
        return;
    }

    d->m_geometry = rect;
    Q_EMIT targetRectChanged();

    invalidateFilter();
}

bool MaximizedWindowMonitor::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0);

    if (!TaskManager::TasksModel::filterAcceptsRow(sourceRow, sourceParent)) {
        return false;
    }

    // Filter windows on the screen where the wallpaper plugin is on
    const QRect windowGeometry = sourceIndex.data(TaskManager::AbstractTasksModel::Geometry).toRect();

    // Use intersects so the geometry doesn't need be multiplied by devicePixelRatio
    if (!d->m_geometry.intersects(windowGeometry)) {
        return false;
    }

    if (sourceIndex.data(TaskManager::AbstractTasksModel::IsMaximized).toBool() || sourceIndex.data(TaskManager::AbstractTasksModel::IsFullScreen).toBool()) {
        return true;
    }

    return false;
}
