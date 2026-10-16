/*
    SPDX-FileCopyrightText: 2024 Kristen McWilliam <kmcwilliampublic@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "abstracttasksmodel.h"
#include "fullscreentracker_p.h"

using namespace NotificationManager;

FullscreenTracker::FullscreenTracker(QObject *parent)
    : QObject(parent)
{
    m_tasks.setFilterMinimized(true);
    m_tasks.setFilterHidden(true);

    checkFullscreenFocused();

    connect(&m_tasks, &TaskManager::TasksModel::activeTaskChanged, this, &FullscreenTracker::checkFullscreenFocused);
    connect(&m_tasks, &TaskManager::TasksModel::dataChanged, this, &FullscreenTracker::checkFullscreenFocused);
}

FullscreenTracker::~FullscreenTracker() = default;

FullscreenTracker::Ptr FullscreenTracker::createTracker()
{
    static std::weak_ptr<FullscreenTracker> s_instance;
    if (s_instance.expired()) {
        std::shared_ptr<FullscreenTracker> ptr(new FullscreenTracker(nullptr));
        s_instance = ptr;
        return ptr;
    }
    return s_instance.lock();
}

bool FullscreenTracker::fullscreenFocused() const
{
    return m_fullscreenFocused;
}

void FullscreenTracker::setFullscreenFocused(bool focused)
{
    if (m_fullscreenFocused != focused) {
        m_fullscreenFocused = focused;
        Q_EMIT fullscreenFocusedChanged(focused);
    }
}

void FullscreenTracker::checkFullscreenFocused()
{
    QModelIndex activeTaskIndex = m_tasks.activeTask();
    if (!activeTaskIndex.isValid()) {
        setFullscreenFocused(false);
        return;
    }

    bool isFullscreen = activeTaskIndex.data(TaskManager::AbstractTasksModel::IsFullScreen).toBool();

    setFullscreenFocused(isFullscreen);
}
