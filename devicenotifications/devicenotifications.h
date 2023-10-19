/*
    SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QHash>
#include <QList>
#include <QSocketNotifier>
#include <QString>

#include <KDEDModule>

#include <libudev.h>

class UdevDevice
{
public:
    explicit UdevDevice(struct udev_device *device);
    ~UdevDevice();
    UdevDevice &operator=(const UdevDevice &other);

    static UdevDevice fromDevice(struct udev_device *device);

    udev_device *handle() const;

    QString action() const;
    QString name() const;
    QString sysfsPath() const;
    QString subsystem() const;
    QString type() const;

    QString deviceProperty(const char *name) const;
    QString sysfsProperty(const char *name) const;

    QString model() const;
    QString vendor() const;
    QString displayName() const;

    bool isRemovable() const;

private:
    UdevDevice(struct udev_device *device, bool ref);

    QString getDeviceString(const char *(*getter)(struct udev_device *)) const;

    struct udev_device *m_device = nullptr;
};

class Udev : public QObject
{
    Q_OBJECT

public:
    Udev(QObject *parent = nullptr);
    ~Udev() override;

Q_SIGNALS:
    void deviceAdded(const UdevDevice &device);
    void deviceRemoved(const UdevDevice &device);

private:
    void onSocketActivated();

    struct udev *m_udev = nullptr;
    struct udev_monitor *m_monitor = nullptr;
    QSocketNotifier *m_notifier = nullptr;
};

class KdedDeviceNotifications : public KDEDModule
{
    Q_OBJECT

public:
    KdedDeviceNotifications(QObject *parent, const QVariantList &args);
    ~KdedDeviceNotifications() override;

private:
    void onDeviceAdded(const UdevDevice &device);
    void onDeviceRemoved(const UdevDevice &device);

    Udev m_udev;
    QHash<QString, QString> m_displayNames;
    QList<QString> m_removableDevices;
};
