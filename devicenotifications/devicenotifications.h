/*
    SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QHash>
#include <QList>
#include <QPointer>
#include <QSocketNotifier>
#include <QString>
#include <QTimer>

#include <KDEDModule>
#include <KNotification>

#include "qwayland-kde-output-device-v2.h"
#include <libudev.h>

struct wl_registry;

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

class Output : public QObject, public QtWayland::kde_output_device_v2
{
    Q_OBJECT
public:
    Output(uint32_t id);
    ~Output();

    uint32_t id() const
    {
        return m_id;
    }

    QString uuid() const
    {
        return m_uuid;
    }

Q_SIGNALS:
    void uuidAdded();

private:
    void kde_output_device_v2_uuid(const QString &uuid) override;
    uint32_t m_id;
    QString m_uuid;
};

class KdedDeviceNotifications : public KDEDModule
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.plasma.devicenotifications")

public:
    KdedDeviceNotifications(QObject *parent, const QVariantList &args);
    ~KdedDeviceNotifications() override;

    void setupWaylandOutputListener();

    Q_SCRIPTABLE void dismissUsbDeviceAdded();

private:
    void notifyOutputAdded();
    void notifyOutputRemoved();
    void onDeviceAdded(const UdevDevice &device);
    void onDeviceRemoved(const UdevDevice &device);

    Udev m_udev;
    QHash<QString, QString> m_displayNames;
    QList<QString> m_removableDevices;

    wl_registry *m_registry = nullptr;
    std::vector<std::unique_ptr<Output>> m_outputs;
    QList<QString> m_recentlyRemovedOutputs;
    bool m_initialOutputsReceived = false;

    QTimer m_deviceAddedTimer;
    QTimer m_deviceRemovedTimer;

    QPointer<KNotification> m_usbDeviceAddedNotification;
    QPointer<KNotification> m_usbDeviceRemovedNotification;

    QPointer<KNotification> m_displayAddedNotification;
    QPointer<KNotification> m_displayRemovedNotification;
};
