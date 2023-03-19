/*
    SPDX-FileCopyrightText: 2023 David Edmundson <davidedmundson@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <chrono>
#include <memory>

class AlignedTimer : public QObject
{
    Q_OBJECT
public:
    explicit AlignedTimer(std::chrono::seconds interval);
    ~AlignedTimer() override;

    static std::shared_ptr<AlignedTimer> getMinuteTimer();
    static std::shared_ptr<AlignedTimer> getSecondTimer();

Q_SIGNALS:
    void timeout();

private:
    void initTimer();
    std::chrono::seconds m_interval;
    int m_timerFd = -1;
};
