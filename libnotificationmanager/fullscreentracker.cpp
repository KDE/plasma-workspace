/*
    SPDX-FileCopyrightText: 2024 Kristen McWilliam <kmcwilliampublic@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "abstracttasksmodel.h"
#include "fullscreentracker_p.h"

#include <QGuiApplication>
#include <QWindow>

#include <KWindowInfo>
#include <KWindowSystem>
#include <qguiapplication.h>

using namespace NotificationManager;

FullscreenTracker::FullscreenTracker(QObject *parent)
    : TaskManager::TasksModel(parent)
{
    setFilterMinimized(true);
    setFilterHidden(true);

    checkFullscreenActive();

    connect(m_tasksModel, &TaskManager::TasksModel::activeTaskChanged, this, &FullscreenTracker::checkFullscreenActive);
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

bool FullscreenTracker::fullscreenActive() const
{
    return m_fullscreenActive;
}

void FullscreenTracker::setFullscreenActive(bool active)
{
    if (m_fullscreenActive != active) {
        m_fullscreenActive = active;
        Q_EMIT fullscreenActiveChanged(active);
    }
}

void FullscreenTracker::checkFullscreenActive()
{
    qDebug() << "Checking fullscreen active";

    QModelIndex activeTaskIndex = m_tasksModel->activeTask();
    if (!activeTaskIndex.isValid()) {
        setFullscreenActive(false);
        return;
    }

    QString appId = activeTaskIndex.data(TaskManager::AbstractTasksModel::AppId).toString();
    QString appName = activeTaskIndex.data(TaskManager::AbstractTasksModel::AppName).toString();
    QString GenericName = activeTaskIndex.data(TaskManager::AbstractTasksModel::GenericName).toString();

    bool isFullscreen = activeTaskIndex.data(TaskManager::AbstractTasksModel::IsFullScreen).toBool();

    qDebug() << "AppId: " << appId << " AppName: " << appName << " GenericName: " << GenericName << " IsFullscreen: " << isFullscreen;
    setFullscreenActive(isFullscreen);
}
