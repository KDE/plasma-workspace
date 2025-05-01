/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <memory>

#include <QObject>

#include "tasksmodel.h"

/**
 * This class monitors if there is any maximized or fullscreen window.
 * It is used by the animated image component.
 */
class MaximizedWindowMonitor : public TaskManager::TasksModel
{
    Q_OBJECT

public:
    explicit MaximizedWindowMonitor(QObject *parent = nullptr);
    ~MaximizedWindowMonitor() override;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    std::shared_ptr<TaskManager::ActivityInfo> m_activityInfo;
    std::shared_ptr<TaskManager::VirtualDesktopInfo> m_virtualDesktopInfo;
};
