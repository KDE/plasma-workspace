/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QHash>
#include <QTimer>

#include <devicestatemonitor_p.h>

/**
 * This class monitors the full and free size of devices
 */

class SpaceMonitor : public QObject
{
    Q_OBJECT

public:
    static std::shared_ptr<SpaceMonitor> instance();
    ~SpaceMonitor() override;

    void setIsVisible(bool status);

    void addMonitoringDevice(const QString &udi);
    void removeMonitoringDevice(const QString &udi);
    void forceUpdateSize(const QString &udi);

    double getFullSize(const QString &udi) const;
    double getFreeSize(const QString &udi) const;

private:
    explicit SpaceMonitor(QObject *parent = nullptr);

private Q_SLOTS:
    void deviceStateChanged(QString udi);
    void updateAllStorageSpaces();

Q_SIGNALS:
    void sizeChanged(const QString &udi);

private:
    void updateStorageSpace(const QString &udi);

    QHash<QString, std::pair<double, double>> m_sizes;
    std::shared_ptr<DevicesStateMonitor> m_stateMonitor;
    QTimer *m_spaceWatcher;
};
