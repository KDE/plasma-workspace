/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QHash>
#include <QObject>

#include <Solid/SolidNamespace>

class DevicesStateMonitor;

/**
 * Class that monitors error messages for devices
 */
class DeviceMessageMonitor : public QObject
{
    Q_OBJECT

public:
    static std::shared_ptr<DeviceMessageMonitor> instance();
    ~DeviceMessageMonitor() override;

    void addMonitoringDevice(const QString &udi);
    void removeMonitoringDevice(const QString &udi);

    QString getMessage(const QString &udi);

private:
    explicit DeviceMessageMonitor(QObject *parent = nullptr);

Q_SIGNALS:
    void messageChanged(const QString &udi);

    void blockingAppsReady(const QStringList &apps);

private Q_SLOTS:
    void onStateChanged(const QString &udi);
    void notify(const QString &message, const QString &info, const QString &udi);
    bool isSafelyRemovable(const QString &udi) const;
    void queryBlockingApps(const QString &devicePath);
    void clearPreviousMessage(const QString &udi);

private:
    QHash<QString, QString> m_deviceMessages;

    std::shared_ptr<DevicesStateMonitor> m_deviceStateMonitor;
};
