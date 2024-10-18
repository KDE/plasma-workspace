/*
    SPDX-FileCopyrightText: 2024 Kristen McWilliam <kmcwilliampublic@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QObject>
#include <memory>

namespace NotificationManager
{

class FullscreenTracker : public QObject
{
    Q_OBJECT

public:
    ~FullscreenTracker();

    using Ptr = std::shared_ptr<FullscreenTracker>;
    static Ptr createTracker();

    bool fullscreenActive() const;
    void setFullscreenActive(bool active);
    Q_SIGNAL void fullscreenActiveChanged(bool active);

private:
    FullscreenTracker();
    Q_DISABLE_COPY(FullscreenTracker)

    void checkFullscreenActive();

    bool m_fullscreenActive = false;
};

}
