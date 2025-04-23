/*
    SPDX-FileCopyrightText: 2024 Kristen McWilliam <kmcwilliampublic@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QObject>

#include <tasksmodel.h>

namespace NotificationManager
{

/**
 * Tracks whether a fullscreen window is focused.
 *
 * It is used to determine whether to automatically enable Do Not Disturb mode when a fullscreen window is focused.
 *
 **/
class FullscreenTracker : public TaskManager::TasksModel
{
    Q_OBJECT

public:
    FullscreenTracker(QObject *parent = nullptr);
    ~FullscreenTracker() override;

    using Ptr = std::shared_ptr<FullscreenTracker>;
    static Ptr createTracker();

    bool fullscreenFocused() const;
    void setFullscreenFocused(bool focused);
    Q_SIGNAL void fullscreenFocusedChanged(bool focused);

private:
    FullscreenTracker();
    Q_DISABLE_COPY(FullscreenTracker)

    void checkFullscreenFocused();
    bool m_fullscreenFocused = false;
};

}
