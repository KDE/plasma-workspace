/*
    SPDX-FileCopyrightText: 2024 Kristen McWilliam <kmcwilliampublic@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "abstracttasksmodel.h"
#include "fullscreentracker_p.h"

using namespace NotificationManager;

FullscreenTracker::FullscreenTracker(QObject *parent)
    : TaskManager::TasksModel(parent)
{
    setFilterMinimized(true);
    setFilterHidden(true);

    checkFullscreenFocused();

    connect(this, &TaskManager::TasksModel::activeTaskChanged, this, &FullscreenTracker::checkFullscreenFocused);
    connect(this, &TaskManager::TasksModel::dataChanged, this, &FullscreenTracker::checkFullscreenFocused);
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
    QModelIndex activeTaskIndex = activeTask();
    if (!activeTaskIndex.isValid()) {
        setFullscreenFocused(false);
        return;
    }

    bool isFullscreen = activeTaskIndex.data(TaskManager::AbstractTasksModel::IsFullScreen).toBool();

    setFullscreenFocused(isFullscreen);
}
