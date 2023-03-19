/*
    SPDX-FileCopyrightText: 2023 David Edmundson <davidedmundson@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>
#include <memory>

#pragma once

class AlignedTimer : public QObject
{
    Q_OBJECT
public:
    AlignedTimer(int interval);
    ~AlignedTimer();

    static std::shared_ptr<AlignedTimer> getMinuteTimer();
    static std::shared_ptr<AlignedTimer> getSecondTimer();

Q_SIGNALS:
    void timeout();

private:
    void initTimer();
    int m_interval = 1;
    int m_timerFd = -1;
};
