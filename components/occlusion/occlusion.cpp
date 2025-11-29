/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "occlusion.h"
#include "abstracttasksmodel.h"
#include "activityinfo.h"
#include "virtualdesktopinfo.h"

Occlusion::Occlusion(QObject *parent)
    : QObject(parent)
    , m_tasksModel(new TaskManager::TasksModel(this))
{
    m_tasksModel->setFilterByVirtualDesktop(true);
    m_tasksModel->setFilterByActivity(true);
    m_tasksModel->setFilterByScreen(false);
    m_tasksModel->setFilterByRegion(RegionFilterMode::Intersect);
    m_tasksModel->setFilterHidden(true);
    m_tasksModel->setFilterMinimized(true);
    m_tasksModel->setGroupMode(TaskManager::TasksModel::GroupDisabled);

    auto activityInfo = new TaskManager::ActivityInfo(this);
    m_tasksModel->setActivity(activityInfo->currentActivity());
    connect(activityInfo, &TaskManager::ActivityInfo::currentActivityChanged, this, [activityInfo, this]() {
        m_tasksModel->setActivity(activityInfo->currentActivity());
    });

    auto desktopInfo = new TaskManager::VirtualDesktopInfo(this);
    m_tasksModel->setVirtualDesktop(desktopInfo->currentDesktop());
    connect(desktopInfo, &TaskManager::VirtualDesktopInfo::currentDesktopChanged, this, [desktopInfo, this]() {
        m_tasksModel->setVirtualDesktop(desktopInfo->currentDesktop());
    });

    connect(m_tasksModel, &TaskManager::TasksModel::modelReset, this, &Occlusion::update);
    connect(m_tasksModel, &TaskManager::TasksModel::rowsInserted, this, &Occlusion::update);
    connect(m_tasksModel, &TaskManager::TasksModel::rowsRemoved, this, &Occlusion::update);
    connect(m_tasksModel, &TaskManager::TasksModel::dataChanged, this, [this](const QModelIndex &, const QModelIndex &, const QList<int> &roles) {
        if (roles.isEmpty() || roles.contains(TaskManager::AbstractTasksModel::IsFullScreen)) {
            update();
        }
    });
}

bool Occlusion::coveringAny() const
{
    return m_coveringAny;
}

bool Occlusion::coveringFullScreen() const
{
    return m_coveringFullScreen;
}

QRect Occlusion::area() const
{
    return m_tasksModel->regionGeometry();
}

void Occlusion::setArea(const QRect &area)
{
    if (m_tasksModel->regionGeometry() == area) {
        return;
    }

    m_tasksModel->setRegionGeometry(area);
    Q_EMIT areaChanged();
}

void Occlusion::update()
{
    const int count = m_tasksModel->rowCount();
    setCoveringAny(count);

    bool fullscreen = false;
    for (int i = 0; i < count; ++i) {
        const QModelIndex index = m_tasksModel->index(i, 0);
        if (index.data(TaskManager::AbstractTasksModel::IsFullScreen).toBool()) {
            fullscreen = true;
            break;
        }
    }
    setCoveringFullScreen(fullscreen);
}

void Occlusion::setCoveringAny(bool covering)
{
    if (m_coveringAny != covering) {
        m_coveringAny = covering;
        Q_EMIT coveringAnyChanged();
    }
}

void Occlusion::setCoveringFullScreen(bool covering)
{
    if (m_coveringFullScreen != covering) {
        m_coveringFullScreen = covering;
        Q_EMIT coveringFullScreenChanged();
    }
}

#include "moc_occlusion.cpp"
