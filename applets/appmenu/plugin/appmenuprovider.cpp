/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "appmenuprovider.h"

#include "abstracttasksmodel.h"

AppMenuProvider::AppMenuProvider(QObject *parent)
    : QObject(parent)
    , m_tasks(std::make_unique<TaskManager::TasksModel>())
    , m_timer(std::make_unique<QTimer>())
{
    m_tasks->setFilterByScreen(true);
    connect(m_tasks.get(), &TaskManager::TasksModel::rowsInserted, this, &AppMenuProvider::onTasksRowsInserted);
    connect(m_tasks.get(), &TaskManager::TasksModel::rowsRemoved, this, &AppMenuProvider::onTasksRowsRemoved);
    connect(m_tasks.get(), &TaskManager::TasksModel::modelReset, this, &AppMenuProvider::onTasksModelReset);
    connect(m_tasks.get(), &TaskManager::TasksModel::dataChanged, this, &AppMenuProvider::onTasksDataChanged);
    connect(m_tasks.get(), &TaskManager::TasksModel::screenGeometryChanged, this, &AppMenuProvider::screenGeometryChanged);

    // This timer prevents flicker when the active window changes.
    m_timer->setSingleShot(true);
    m_timer->setInterval(500);
    connect(m_timer.get(), &QTimer::timeout, this, &AppMenuProvider::apply);

    tryReload();
}

DBusMenuModel *AppMenuProvider::model() const
{
    return m_current.get();
}

QRect AppMenuProvider::screenGeometry() const
{
    return m_tasks->screenGeometry();
}

void AppMenuProvider::setScreenGeometry(const QRect &geometry)
{
    m_tasks->setScreenGeometry(geometry);
}

void AppMenuProvider::onTasksRowsInserted()
{
    tryReload();
}

void AppMenuProvider::onTasksRowsRemoved()
{
    tryReload();
}

void AppMenuProvider::onTasksModelReset()
{
    tryReload();
}

void AppMenuProvider::onTasksDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    Q_UNUSED(topLeft)
    Q_UNUSED(bottomRight)

    if (roles.isEmpty() || roles.contains(TaskManager::AbstractTasksModel::IsActive)
        || roles.contains(TaskManager::AbstractTasksModel::ApplicationMenuServiceName)
        || roles.contains(TaskManager::AbstractTasksModel::ApplicationMenuServiceName)) {
        tryReload();
    }
}

void AppMenuProvider::tryReload()
{
    const QModelIndex activeTaskIndex = m_tasks->activeTask();
    const QString objectPath = m_tasks->data(activeTaskIndex, TaskManager::AbstractTasksModel::ApplicationMenuObjectPath).toString();
    const QString serviceName = m_tasks->data(activeTaskIndex, TaskManager::AbstractTasksModel::ApplicationMenuServiceName).toString();

    if (m_current && m_current->serviceName() == serviceName && m_current->objectPath() == objectPath) {
        m_next.reset();
        m_timer->stop();
        return;
    }

    if (!m_next || m_next->serviceName() != serviceName || m_next->objectPath() != objectPath) {
        reload(serviceName, objectPath);
    }
}

void AppMenuProvider::reload(const QString &serviceName, const QString &objectPath)
{
    if (serviceName.isEmpty() || objectPath.isEmpty()) {
        m_next.reset();
    } else {
        m_next = std::make_unique<DBusMenuModel>(serviceName, objectPath);
        m_next->setPrefetchSize(2);
        m_next->open(QModelIndex());

        connect(m_next.get(), &DBusMenuModel::rowsInserted, this, &AppMenuProvider::apply);
    }

    if (m_current || m_next) {
        m_timer->start();
    }
}

void AppMenuProvider::apply()
{
    m_current = std::move(m_next);
    if (m_current) {
        disconnect(m_current.get(), &DBusMenuModel::rowsInserted, this, &AppMenuProvider::apply);
        connect(m_current.get(), &DBusMenuModel::activateRequested, this, &AppMenuProvider::activateRequested);
    }

    m_timer->stop();
    Q_EMIT modelChanged();
}

#include "moc_appmenuprovider.cpp"
