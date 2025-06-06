/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class IdleTimeout : public QObject
{
    Q_OBJECT

public:
    explicit IdleTimeout(std::chrono::milliseconds interval, QObject *parent = nullptr);
    ~IdleTimeout() override;

Q_SIGNALS:
    void timeout();

private:
    int m_notifierId;
};
