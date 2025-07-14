/*
 * SPDX-FileCopyrightText: 2025 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>

class QTimer;

class SpaceUpdateMonitor : public QObject
{
    Q_OBJECT

public:
    static std::shared_ptr<SpaceUpdateMonitor> instance();
    ~SpaceUpdateMonitor() override;

    void setIsVisible(bool visible);

Q_SIGNALS:
    void updateSpace();

private:
    explicit SpaceUpdateMonitor(QObject *parent = nullptr);

    int m_usageCount;
    QTimer *m_spaceWatcher;
};
