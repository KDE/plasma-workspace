/*
    SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "devicenotifications.h"

#include <QGuiApplication>
#include <QSocketNotifier>

#include <KLocalizedString>
#include <KNotification>
#include <KPluginFactory>

#include <chrono>

#include <knotification.h>
#include <wayland-client.h>

K_PLUGIN_CLASS_WITH_JSON(KdedDeviceNotifications, "devicenotifications.json")

using namespace std::chrono_literals;

// TODO Can we put this in KStringHandler?
static QString decodePropertyValue(QByteArrayView encoded)
{
    const int len = encoded.length();
    QByteArray decoded;
    // The resulting string is definitely same length or shorter.
    decoded.reserve(len);

    for (int i = 0; i < len; ++i) {
        const auto ch = encoded.at(i);

        if (ch == '\\') {
            if (i + 1 < len && encoded.at(i + 1) == '\\') {
                decoded.append('\\');
                ++i;
            } else if (i + 3 < len && encoded.at(i + 1) == 'x') {
                QByteArrayView hex = encoded.mid(i + 2, 2);
                bool ok;
                const int code = hex.toInt(&ok, 16);
                if (ok) {
                    decoded.append(char(code));
                }
                i += 3;
            }
        } else {
            decoded.append(ch);
        }
    }
    return QString::fromUtf8(decoded);
}

UdevDevice::UdevDevice(struct udev_device *device)
    : UdevDevice(device, true /*ref*/)
{
}

UdevDevice::UdevDevice(struct udev_device *device, bool ref)
    : m_device(device)
{
    if (ref) {
        udev_device_ref(m_device);
    }
}

UdevDevice UdevDevice::fromDevice(struct udev_device *device)
{
    return UdevDevice(device, false /*ref*/);
}

UdevDevice::~UdevDevice()
{
    if (m_device) {
        udev_device_unref(m_device);
    }
}

UdevDevice &UdevDevice::operator=(const UdevDevice &other)
{
    udev_device_unref(m_device);
    m_device = udev_device_ref(other.m_device);
    return *this;
}

struct udev_device *UdevDevice::handle() const
{
    return m_device;
}

QString UdevDevice::action() const
{
    return getDeviceString(udev_device_get_action);
}

QString UdevDevice::name() const
{
    return getDeviceString(udev_device_get_sysname);
}

QString UdevDevice::sysfsPath() const
{
    return getDeviceString(udev_device_get_syspath);
}

QString UdevDevice::subsystem() const
{
    return getDeviceString(udev_device_get_subsystem);
}

QString UdevDevice::type() const
{
    return getDeviceString(udev_device_get_devtype);
}

QString UdevDevice::deviceProperty(const char *name) const
{
    if (m_device) {
        const auto *value = udev_device_get_property_value(m_device, name);
        if (value) {
            return QString::fromLatin1(value);
        }
    }
    return {};
}

QByteArray UdevDevice::devicePropertyRaw(const char *name) const
{
    if (m_device) {
        const auto *value = udev_device_get_property_value(m_device, name);
        if (value) {
            return value;
        }
    }
    return {};
}

QString UdevDevice::sysfsProperty(const char *name) const
{
    if (m_device) {
        const auto *value = udev_device_get_sysattr_value(m_device, name);
        if (value) {
            return QString::fromLatin1(value);
        }
    }
    return {};
}

QString UdevDevice::getDeviceString(const char *(*getter)(udev_device *)) const
{
    if (m_device) {
        return QString::fromLatin1((*getter)(m_device));
    }
    return QString();
}

QString UdevDevice::model() const
{
    QString name = sysfsProperty("product");
    if (name.isEmpty()) {
        name = deviceProperty("ID_MODEL_FROM_DATABASE");
    }
    if (name.isEmpty()) {
        name = decodePropertyValue(devicePropertyRaw("ID_MODEL_ENC"));
    }
    if (name.isEmpty()) {
        name = deviceProperty("ID_MODEL");
    }
    return name;
}

QString UdevDevice::vendor() const
{
    QString vendor = sysfsProperty("manufacturer");
    if (vendor.isEmpty()) {
        vendor = deviceProperty("ID_VENDOR_FROM_DATABASE");
    }
    if (vendor.isEmpty()) {
        vendor = decodePropertyValue(devicePropertyRaw("ID_VENDOR_ENC"));
    }
    if (vendor.isEmpty()) {
        vendor = deviceProperty("ID_VENDOR");
    }
    return vendor;
}

QString UdevDevice::displayName() const
{
    const QString generic = QStringLiteral("Generic");

    QStringList displayNameSegments;

    QString vendor = this->vendor();
    if (vendor == generic) {
        vendor.clear();
    }
    if (!vendor.isEmpty()) {
        displayNameSegments.append(vendor);
    }

    QString model = this->model();
    if (model == generic) {
        model.clear();
    }
    if (!model.isEmpty()) {
        displayNameSegments.append(model);
    }

    return displayNameSegments.join(QLatin1Char(' '));
}

bool UdevDevice::isRemovable() const
{
    // "removable" is a device that can be removed from the platform by the user.
    // See https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-devices-removable
    return sysfsProperty("removable") == QLatin1String("removable");
}

Udev::Udev(QObject *parent)
    : QObject(parent)
    , m_udev(udev_new())
{
    if (!m_udev) {
        return;
    }

    m_monitor = udev_monitor_new_from_netlink(m_udev, "udev");
    if (!m_monitor) {
        return;
    }

    // For now we're only interested in devices on the "usb" subsystem.
    udev_monitor_filter_add_match_subsystem_devtype(m_monitor, "usb", nullptr);

    m_notifier = new QSocketNotifier(udev_monitor_get_fd(m_monitor), QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &Udev::onSocketActivated);

    udev_monitor_enable_receiving(m_monitor);
}

Udev::~Udev()
{
    if (m_monitor) {
        udev_monitor_unref(m_monitor);
    }
    if (m_udev) {
        udev_unref(m_udev);
    }
}

void Udev::onSocketActivated()
{
    m_notifier->setEnabled(false);
    UdevDevice device = UdevDevice::fromDevice(udev_monitor_receive_device(m_monitor));
    m_notifier->setEnabled(true);

    const QString action = device.action();

    if (action == QLatin1String("add")) {
        Q_EMIT deviceAdded(device);
    } else if (action == QLatin1String("remove")) {
        Q_EMIT deviceRemoved(device);
    }
}

Output::Output(uint32_t id)
    : QObject()
    , kde_output_device_v2()
    , m_id(id)
{
}

Output::~Output()
{
    kde_output_device_v2_destroy(object());
}

void Output::kde_output_device_v2_uuid(const QString &uuid)
{
    m_uuid = uuid;
    Q_EMIT uuidAdded();
}

KdedDeviceNotifications::KdedDeviceNotifications(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
{
    // Suppress changes in quick succession in case of (un)plugging a docking station with several outputs and USB devices.
    m_deviceAddedTimer.setInterval(500ms);
    m_deviceAddedTimer.setSingleShot(true);

    m_deviceRemovedTimer.setInterval(500ms);
    m_deviceRemovedTimer.setSingleShot(true);

    connect(&m_udev, &Udev::deviceAdded, this, &KdedDeviceNotifications::onDeviceAdded);
    connect(&m_udev, &Udev::deviceRemoved, this, &KdedDeviceNotifications::onDeviceRemoved);

    setupWaylandOutputListener();
}

KdedDeviceNotifications::~KdedDeviceNotifications()
{
    if (m_registry) {
        wl_registry_destroy(m_registry);
    }
}

void KdedDeviceNotifications::setupWaylandOutputListener()
{
    auto waylandApp = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
    if (!waylandApp) {
        return;
    }

    wl_display *display = waylandApp->display();

    m_registry = wl_display_get_registry(display);

    auto globalAdded = [](void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
        auto *self = static_cast<KdedDeviceNotifications *>(data);
        if (qstrcmp(interface, "kde_output_device_v2") == 0) {
            const bool initialOutputsReceived = self->m_initialOutputsReceived;
            auto &out = self->m_outputs.emplace_back(std::make_unique<Output>(name));
            // Notify after the UUID's are resolved, and add it to our timed list
            connect(out.get(), &Output::uuidAdded, self, [&out, self, initialOutputsReceived]() {
                if (initialOutputsReceived) {
                    const QString uuid = out->uuid();
                    // If we recently just removed this output, it wasn't actually physically disconnected
                    if (!self->m_recentlyRemovedOutputs.removeOne(uuid)) {
                        self->notifyOutputAdded();
                    }
                }
            });
            out->init(registry, name, version);
        }
    };
    auto globalRemoved = [](void *data, wl_registry *registry, uint32_t name) {
        Q_UNUSED(registry)
        auto *self = static_cast<KdedDeviceNotifications *>(data);
        auto result = std::ranges::find_if(self->m_outputs.begin(), self->m_outputs.end(), [name](std::unique_ptr<Output> &out) {
            return out.get()->id() == name;
        });
        if (result != self->m_outputs.end()) {
            auto out = result.base()->get();
            const QString uuid = out->uuid();
            self->m_recentlyRemovedOutputs.append(uuid);
            // 2000ms matches the DPMS workaround time in KWin
            QTimer::singleShot(2000ms, self, [self, uuid]() {
                // Only notify if the output hasn't been added again in the mean time
                if (self->m_recentlyRemovedOutputs.removeOne(uuid)) {
                    self->notifyOutputRemoved();
                }
            });
            self->m_outputs.erase(result);
        }
    };

    static const wl_registry_listener registryListener{globalAdded, globalRemoved};
    wl_registry_add_listener(m_registry, &registryListener, this);

    // Suppress notifications until the inital list of outputs has been received.
    auto syncDone = [](void *data, struct wl_callback *wl_callback, uint32_t callback_data) {
        Q_UNUSED(wl_callback);
        Q_UNUSED(callback_data);
        auto *self = static_cast<KdedDeviceNotifications *>(data);
        self->m_initialOutputsReceived = true;
    };
    auto syncCallback = wl_display_sync(display);
    static const wl_callback_listener syncCallbackListener{syncDone};
    wl_callback_add_listener(syncCallback, &syncCallbackListener, this);
}

void KdedDeviceNotifications::dismissUsbDeviceAdded()
{
    if (m_usbDeviceAddedNotification) {
        m_usbDeviceAddedNotification->close();
        m_usbDeviceAddedNotification = nullptr;
    }
}

void KdedDeviceNotifications::notifyOutputAdded()
{
    if (m_deviceAddedTimer.isActive()) {
        return;
    }

    if (m_displayRemovedNotification) {
        m_displayRemovedNotification->close();
        m_displayRemovedNotification = nullptr;
    }

    m_displayAddedNotification = new KNotification(QStringLiteral("deviceAdded"));
    m_displayAddedNotification->setFlags(KNotification::DefaultEvent);
    m_displayAddedNotification->setIconName(QStringLiteral("video-display-add"));
    m_displayAddedNotification->setTitle(i18nc("@title:notifications", "Display Detected"));
    m_displayAddedNotification->setText(i18n("A display has been connected."));
    m_displayAddedNotification->sendEvent();

    m_deviceAddedTimer.start();
}

void KdedDeviceNotifications::notifyOutputRemoved()
{
    if (m_deviceRemovedTimer.isActive()) {
        return;
    }

    if (m_displayAddedNotification) {
        m_displayAddedNotification->close();
        m_displayAddedNotification = nullptr;
    }

    m_displayRemovedNotification = new KNotification(QStringLiteral("deviceRemoved"));
    m_displayRemovedNotification->setFlags(KNotification::DefaultEvent);
    m_displayRemovedNotification->setIconName(QStringLiteral("video-display-remove"));
    m_displayRemovedNotification->setTitle(i18nc("@title:notifications", "Display Removed"));
    m_displayRemovedNotification->setText(i18n("A display has been disconnected."));
    m_displayRemovedNotification->sendEvent();

    m_deviceRemovedTimer.start();
}

void KdedDeviceNotifications::onDeviceAdded(const UdevDevice &device)
{
    // We only care about actual USB devices, no interfaces, controllers, etc.
    if (device.type() != QLatin1String("usb_device")) {
        return;
    }

    if (!device.isRemovable()) {
        return;
    }

    // By the time we receive the "removed" signal, the device's properties
    // are already discarded, so we need to remember whether it is removable
    // here, so we know that when the device is removed, likewise remember its name.
    m_removableDevices.append(device.sysfsPath());

    const QString displayName = device.displayName();
    if (!displayName.isEmpty()) {
        m_displayNames.insert(device.sysfsPath(), displayName);
    }

    if (m_deviceAddedTimer.isActive()) {
        return;
    }

    // If the user unplugged something and then immediately plugged it in again,
    // there's no need to keep the unplug notification around.
    if (m_usbDeviceRemovedNotification) {
        m_usbDeviceRemovedNotification->close();
        m_usbDeviceRemovedNotification = nullptr;
    }

    // Only show one of these at a time. We already suppressed creating a bunch
    // in quick succession for the dock/hub use case, so any that are created
    // over that time limit anyway are not necessary to stack up.
    if (m_usbDeviceAddedNotification) {
        m_usbDeviceAddedNotification->close();
        m_usbDeviceAddedNotification = nullptr;
    }

    const QString text = !displayName.isEmpty() ? i18n("%1 has been connected.", displayName.toHtmlEscaped()) : i18n("A USB device has been connected.");

    m_usbDeviceAddedNotification = new KNotification(QStringLiteral("deviceAdded"));
    m_usbDeviceAddedNotification->setFlags(KNotification::DefaultEvent);
    m_usbDeviceAddedNotification->setIconName(QStringLiteral("drive-removable-media-usb"));
    m_usbDeviceAddedNotification->setTitle(i18nc("@title:notifications", "USB Device Detected"));
    m_usbDeviceAddedNotification->setText(text);
    m_usbDeviceAddedNotification->sendEvent();

    m_deviceAddedTimer.start();
}

void KdedDeviceNotifications::onDeviceRemoved(const UdevDevice &device)
{
    if (device.type() != QLatin1String("usb_device")) {
        return;
    }

    const QString displayName = m_displayNames.take(device.sysfsPath());

    if (!m_removableDevices.removeOne(device.sysfsPath()) && !device.isRemovable()) {
        return;
    }

    if (m_deviceRemovedTimer.isActive()) {
        return;
    }

    // If the user plugged something in and then immediately unplugged it again,
    // there's no need to keep the plug notification around.
    if (m_usbDeviceAddedNotification) {
        m_usbDeviceAddedNotification->close();
        m_usbDeviceAddedNotification = nullptr;
    }

    // Only show one of these at a time. We already suppressed removing a bunch
    // in quick succession for the dock/hub use case, so any that are removed
    // over that time limit anyway are not necessary to stack up.
    if (m_usbDeviceRemovedNotification) {
        m_usbDeviceRemovedNotification->close();
        m_usbDeviceRemovedNotification = nullptr;
    }

    const QString text = !displayName.isEmpty() ? i18n("%1 has been disconnected.", displayName.toHtmlEscaped()) : i18n("A USB device has been disconnected.");

    m_usbDeviceRemovedNotification = new KNotification(QStringLiteral("deviceRemoved"));
    m_usbDeviceRemovedNotification->setFlags(KNotification::DefaultEvent);
    m_usbDeviceRemovedNotification->setIconName(QStringLiteral("drive-removable-media-usb"));
    m_usbDeviceRemovedNotification->setTitle(i18nc("@title:notifications", "USB Device Went Away"));
    m_usbDeviceRemovedNotification->setText(text);
    m_usbDeviceRemovedNotification->sendEvent();

    m_deviceRemovedTimer.start();
}

#include "devicenotifications.moc"

#include "moc_devicenotifications.cpp"
