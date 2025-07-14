/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QHash>
#include <QObject>

#include <Solid/SolidNamespace>

#include <storageinfo.h>
#include "stateinfo.h"

/**
 * Class that monitors error messages for devices
 */
class DeviceMessageMonitor : public QObject
{
    Q_OBJECT

public:
    static std::shared_ptr<DeviceMessageMonitor> instance();

    void addMonitoringDevice(const QString &udi, const std::shared_ptr<StateInfo> &info);
    void removeMonitoringDevice(const QString &udi);

    QString getMessage(const QString &udi);

private:
    explicit DeviceMessageMonitor(QObject *parent = nullptr);

Q_SIGNALS:
    void messageChanged(const QString &udi);

    void blockingAppsReady(const QStringList &apps);

private Q_SLOTS:
    void onStateChanged(const QString &udi);
    void notify(const std::optional<QString> &message, const QString &info, const QString &udi);
    void queryBlockingApps(const QString &devicePath);
    void clearPreviousMessage(const QString &udi);

private:
    struct DeviceInfo {
        QString message;
        std::shared_ptr<StateInfo> state;
    };

    QHash<QString, DeviceInfo> m_deviceMessages;
};
