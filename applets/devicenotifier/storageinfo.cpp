/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "storageinfo.h"

#include "devicenotifier_debug.h"

#include <Solid/Block>
#include <Solid/Camera>
#include <Solid/DeviceNotifier>
#include <Solid/OpticalDisc>
#include <Solid/PortableMediaPlayer>
#include <Solid/StorageAccess>
#include <Solid/StorageDrive>

const QList<Solid::DeviceInterface::Type> StorageInfo::m_types({
    Solid::DeviceInterface::PortableMediaPlayer,
    Solid::DeviceInterface::Camera,
    Solid::DeviceInterface::OpticalDisc,
    Solid::DeviceInterface::StorageVolume,
    Solid::DeviceInterface::OpticalDrive,
    Solid::DeviceInterface::StorageDrive,
    Solid::DeviceInterface::NetworkShare,
    Solid::DeviceInterface::StorageAccess,
});

const Solid::Predicate
    StorageInfo::m_encryptedPredicate(Solid::Predicate(QStringLiteral("StorageVolume"), QStringLiteral("usage"), QLatin1String("Encrypted")));

StorageInfo::~StorageInfo()
{
}

StorageInfo::StorageInfo(const QString &udi, QObject *parent)
    : QObject(parent)
    , m_isValid(false)
    , m_isEncrypted(false)
    , m_isRemovable(false)
    , m_hasRemovableParent(false)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Storage Info " << udi << " : begin initializing";

    for (auto type : m_types) {
        m_predicateDeviceMatch |= Solid::Predicate(type);
    }

    Solid::Device device(udi);

    if (!device.isValid()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Storage Info " << udi << " : Attempt to add invalid device";
        return;
    }

    if (!m_predicateDeviceMatch.matches(device)) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Storage Info " << udi << " : is not a storage device";
        return;
    }
    qCDebug(APPLETS::DEVICENOTIFIER) << "Storage Info " << udi << " : begin initializing";

    // Skip things we know we don't care about
    if (device.is<Solid::StorageDrive>()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Storage Info " << udi << " : is a storage drive";
        const Solid::StorageDrive *drive = device.as<Solid::StorageDrive>();
        if (!drive->isHotpluggable()) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Storage Info " << udi << " : is not in our interest. Skipping";
            return;
        }
    } else if (device.is<Solid::StorageVolume>()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Storage Info " << udi << " : is storage volume";

        const Solid::StorageVolume *volume = device.as<Solid::StorageVolume>();
        if (!volume) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Storage Info " << udi << " : is not in our interest. Skipping";
            return;
        }
        Solid::StorageVolume::UsageType type = volume->usage();
        if ((type == Solid::StorageVolume::Unused || type == Solid::StorageVolume::PartitionTable) && !device.is<Solid::OpticalDisc>()) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Storage Info " << udi << " : is not in our interest. Skipping";
            return;
        }
    }

    if (m_encryptedPredicate.matches(device)) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Storage Info " << udi << " : is a encrypted device";
        m_isEncrypted = true;
    }

    if (device.is<Solid::StorageDrive>()) {
        Solid::StorageDrive *storagedrive = device.as<Solid::StorageDrive>();
        if (storagedrive) {
            m_isRemovable = storagedrive->isRemovable();
            qCDebug(APPLETS::DEVICENOTIFIER) << "Storage Info " << udi << " : is " << (m_isRemovable ? "" : "not") << " removable";
        }
    }

    Solid::StorageDrive *drive = getAncestorAs<Solid::StorageDrive>(device);
    if (drive) {
        // remove check for isHotpluggable() when plasmoids are changed to check for both properties
        m_isRemovable = drive->isRemovable() || drive->isHotpluggable();
        qCDebug(APPLETS::DEVICENOTIFIER) << "Storage Info " << udi << " : is " << (m_isRemovable ? "" : "not") << " removable";
    }

    if (device.is<Solid::Camera>()) {
        Solid::Camera *camera = device.as<Solid::Camera>();
        if (camera) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Storage Info " << udi << " : is a camera";
            m_isRemovable = true;
        }
    }

    if (device.is<Solid::PortableMediaPlayer>()) {
        Solid::PortableMediaPlayer *mediaplayer = device.as<Solid::PortableMediaPlayer>();
        if (mediaplayer) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Storage Info " << udi << " : is a media player";
            m_isRemovable = true;
        }
    }

    if (device.is<Solid::StorageVolume>()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Storage Info " << udi << " : parent device: " << device.parent().udi();
        if (m_isRemovable) {
            m_hasRemovableParent = true;
        }
        // Allow mount/unmount only for unused loop devices.
        // Do not unmount a mounted loop device, as it may be in use or system-related.
        if (device.is<Solid::Block>() && device.is<Solid::StorageAccess>()) {
            auto block = device.as<Solid::Block>();
            auto access = device.as<Solid::StorageAccess>();
            if (access && !access->isAccessible() && block && block->device().contains(QStringLiteral("/loop"))) {
                m_isRemovable = true;
            }
        }
    }

    for (auto type : m_types) {
        const Solid::DeviceInterface *interface = device.asDeviceInterface(type);
        if (interface) {
            m_type = Solid::DeviceInterface::typeDescription(type);
            qCDebug(APPLETS::DEVICENOTIFIER) << "Storage Info " << udi << " : type is a " << m_type;
            break;
        }
    }

    m_device = device;

    // Save icon and description of a device because if, use directly from the Solid::Device, they become empty.
    // The applet then, if the device was moved in the model, shows the device with the empty icon and description.
    m_icon = m_device.icon();
    m_description = m_device.description();

    m_isValid = true;

    qCDebug(APPLETS::DEVICENOTIFIER) << "Storage Info " << udi << " : added";
}

bool StorageInfo::isValid() const
{
    return m_isValid;
}

Solid::Predicate StorageInfo::predicate()
{
    Solid::Predicate predicate;
    for (auto type : m_types) {
        predicate |= Solid::Predicate(type);
    }
    return predicate;
}

bool StorageInfo::isEncrypted() const
{
    return m_isEncrypted;
}

bool StorageInfo::hasRemovableParent() const
{
    return m_hasRemovableParent;
}

bool StorageInfo::isRemovable() const
{
    return m_isRemovable;
}

QString StorageInfo::type() const
{
    return m_type;
}

QString StorageInfo::icon() const
{
    return m_icon;
}

QString StorageInfo::description() const
{
    return m_description;
}

const Solid::Device &StorageInfo::device() const
{
    return m_device;
}

#include "moc_storageinfo.cpp"
