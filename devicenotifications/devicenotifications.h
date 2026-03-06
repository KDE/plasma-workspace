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
#include <QWaylandClientExtensionTemplate>

#include <KDEDModule>
#include <KNotification>

#include "qwayland-kde-output-device-v2.h"
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
    QByteArray devicePropertyRaw(const char *name) const;
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

class OutputDevice : public QObject, public QtWayland::kde_output_device_v2
{
    Q_OBJECT

public:
    explicit OutputDevice(::kde_output_device_v2 *outputDevice);
    ~OutputDevice() override;

    bool isInitialized() const
    {
        return m_isInitialized;
    }

    QString uuid() const
    {
        return m_uuid;
    }

Q_SIGNALS:
    void done();
    void removed();

protected:
    void kde_output_device_v2_uuid(const QString &uuid) override;
    void kde_output_device_v2_mode(struct ::kde_output_device_mode_v2 *mode) override;
    void kde_output_device_v2_done() override;
    void kde_output_device_v2_removed() override;

private:
    QString m_uuid;
    bool m_isInitialized = false;
};

class OutputDeviceMode : public QtWayland::kde_output_device_mode_v2
{
public:
    explicit OutputDeviceMode(::kde_output_device_mode_v2 *mode);
    ~OutputDeviceMode();

protected:
    void kde_output_device_mode_v2_removed() override;
};

class OutputDeviceRegistry : public QWaylandClientExtensionTemplate<OutputDeviceRegistry>, public QtWayland::kde_output_device_registry_v2
{
    Q_OBJECT

public:
    OutputDeviceRegistry();
    ~OutputDeviceRegistry() override;

Q_SIGNALS:
    void outputAdded(OutputDevice *output);
    void outputRemoved(OutputDevice *output);

protected:
    void kde_output_device_registry_v2_finished() override;
    void kde_output_device_registry_v2_output(struct ::kde_output_device_v2 *output) override;

private:
    std::vector<std::unique_ptr<OutputDevice>> m_outputDevices;
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

    OutputDeviceRegistry *m_outputRegistry = nullptr;
    QList<QString> m_recentlyRemovedOutputs;
    bool m_initialOutputsReceived = false;

    QTimer m_deviceAddedTimer;
    QTimer m_deviceRemovedTimer;

    QPointer<KNotification> m_usbDeviceAddedNotification;
    QPointer<KNotification> m_usbDeviceRemovedNotification;

    QPointer<KNotification> m_displayAddedNotification;
    QPointer<KNotification> m_displayRemovedNotification;
};
