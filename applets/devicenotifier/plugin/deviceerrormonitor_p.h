/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QHash>
#include <QObject>

#include <Solid/SolidNamespace>

/**
 * Class that monitors error messages for devices
 */

class DeviceErrorMonitor : public QObject
{
    Q_OBJECT

public:
    static std::shared_ptr<DeviceErrorMonitor> instance();
    ~DeviceErrorMonitor() override;

    void addMonitoringDevice(const QString &udi);
    void removeMonitoringDevice(const QString &udi);

    Solid::ErrorType getError(const QString &udi);
    QString getErrorMassage(const QString &udi);

private:
    enum class SolidReplyType {
        Setup = 0,
        Teardown,
        Eject,
    };

    explicit DeviceErrorMonitor(QObject *parent = nullptr);

Q_SIGNALS:
    void errorDataChanged(const QString &udi);

    void blockingAppsReady(const QStringList &apps);

private Q_SLOTS:
    void onSolidReply(SolidReplyType type, Solid::ErrorType error, const QVariant &errorData, const QString &udi);
    void notify(Solid::ErrorType error, const QString &errorMessage, const QString &errorData, const QString &udi);
    bool isSafelyRemovable(const QString &udi) const;
    void queryBlockingApps(const QString &devicePath);

private:
    QHash<QString, std::pair<Solid::ErrorType, QString>> m_deviceErrors;
};
