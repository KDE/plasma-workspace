/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "devicecontrol.h"

#include "devicenotifier_debug.h"

// solid specific includes
#include <Solid/Device>
#include <Solid/DeviceInterface>
#include <Solid/DeviceNotifier>
#include <Solid/GenericInterface>
#include <Solid/OpticalDisc>
#include <Solid/StorageDrive>
#include <Solid/StorageVolume>

#include <KFormat>

#include <QTimer>

DeviceControl::DeviceControl(QObject *parent)
    : QAbstractListModel(parent)
    , m_types({
          Solid::DeviceInterface::PortableMediaPlayer,
          Solid::DeviceInterface::Camera,
          Solid::DeviceInterface::OpticalDisc,
          Solid::DeviceInterface::StorageVolume,
          Solid::DeviceInterface::OpticalDrive,
          Solid::DeviceInterface::StorageDrive,
          Solid::DeviceInterface::NetworkShare,
          Solid::DeviceInterface::StorageAccess,
      })
    , m_encryptedPredicate(Solid::Predicate(QStringLiteral("StorageVolume"), QStringLiteral("usage"), QLatin1String("Encrypted")))
    , m_spaceMonitor(SpaceMonitor::instance())
    , m_stateMonitor(DevicesStateMonitor::instance())
    , m_errorMonitor(DeviceErrorMonitor::instance())

{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Begin initializing";

    for (auto type : m_types) {
        m_predicateDeviceMatch |= Solid::Predicate(type);
    }

    QList<Solid::Device> devices = Solid::Device::listFromQuery(m_predicateDeviceMatch);
    for (Solid::Device &device : devices) {
        onDeviceAdded(device.udi());
    }

    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceAdded, this, &DeviceControl::onDeviceAdded);
    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceRemoved, this, &DeviceControl::onDeviceRemoved);

    connect(m_spaceMonitor.get(), &SpaceMonitor::sizeChanged, this, &DeviceControl::onDeviceSizeChanged);
    connect(m_stateMonitor.get(), &DevicesStateMonitor::stateChanged, this, &DeviceControl::onDeviceStatusChanged);
    connect(m_errorMonitor.get(), &DeviceErrorMonitor::errorDataChanged, this, &DeviceControl::onDeviceErrorChanged);
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Initialized";
}

DeviceControl::~DeviceControl()
{
}

int DeviceControl::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_devices.size();
}

QVariant DeviceControl::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Model : Index is not valid. Role : " << role;
        return {};
    }

    switch (role) {
    case Udi:
        return m_devices[index.row()].udi();
    case Icon:
        return m_deviceTypes[m_devices[index.row()].udi()].second.first;
    case Emblems:
        return m_devices[index.row()].emblems();
    case Description:
        return m_deviceTypes[m_devices[index.row()].udi()].second.second;
    case IsRemovable: {
        return m_stateMonitor->isRemovable(m_devices[index.row()].udi());
    }
    case Size:
        return m_spaceMonitor->getFullSize(m_devices[index.row()].udi());
    case FreeSpace:
        return m_spaceMonitor->getFreeSize(m_devices[index.row()].udi());
    case SizeText: {
        double size = m_spaceMonitor->getFullSize(m_devices[index.row()].udi());
        return size != -1 ? KFormat().formatByteSize(size) : QString();
    }
    case FreeSpaceText: {
        double freeSpace = m_spaceMonitor->getFreeSize(m_devices[index.row()].udi());
        return freeSpace != -1 ? KFormat().formatByteSize(freeSpace) : QString();
    }
    case Mounted: {
        return m_stateMonitor->isMounted(m_devices[index.row()].udi());
    }

    case OperationResult: {
        return m_stateMonitor->getOperationResult(m_devices[index.row()].udi());
    }
    case Timestamp: {
        return m_stateMonitor->getDeviceTimeStamp(m_devices[index.row()].udi());
    }
    case Type: {
        return m_deviceTypes[m_devices[index.row()].udi()].first;
    }
    case Error:
        return m_errorMonitor->getError(m_devices[index.row()].udi());
    case ErrorMessage:
        return m_errorMonitor->getErrorMassage(m_devices[index.row()].udi());
    case Actions: {
        if (auto it = m_actions.constFind(m_devices[index.row()].udi()); it != m_actions.end()) {
            return QVariant::fromValue(*it);
        }
        return {};
    }
    }

    return {};
}

QHash<int, QByteArray> DeviceControl::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Udi] = "deviceUdi";
    roles[Description] = "deviceDescription";
    roles[Type] = "deviceType";
    roles[Icon] = "deviceIcon";
    roles[Emblems] = "deviceEmblems";
    roles[IsRemovable] = "deviceIsRemovable";
    roles[FreeSpace] = "deviceFreeSpace";
    roles[Size] = "deviceSize";
    roles[FreeSpaceText] = "deviceFreeSpaceText";
    roles[SizeText] = "deviceSizeText";
    roles[Mounted] = "deviceMounted";
    roles[OperationResult] = "deviceOperationResult";
    roles[Timestamp] = "deviceTimestamp";
    roles[Error] = "deviceError";
    roles[ErrorMessage] = "deviceErrorMessage";
    roles[Actions] = "deviceActions";
    return roles;
}

void DeviceControl::onDeviceAdded(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Added device signal arrived : " << udi;
    Solid::Device device(udi);

    if (!device.isValid()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Attempt to add invalid device ";
        return;
    }

    if (!m_predicateDeviceMatch.matches(device)) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: device : " << udi << "not in our interest";
        return;
    }
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: New device : " << udi << " begin initializing";

    // Skip things we know we don't care about
    if (device.is<Solid::StorageDrive>()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: device : " << udi << " is storage drive";
        const Solid::StorageDrive *drive = device.as<Solid::StorageDrive>();
        if (!drive->isHotpluggable()) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: device : " << udi << " is not in our interest. Skipping";
            return;
        }
    } else if (device.is<Solid::StorageVolume>()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: device : " << udi << " is storage volume";

        const Solid::StorageVolume *volume = device.as<Solid::StorageVolume>();
        if (!volume) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: device : " << udi << " is not in our interest. Skipping";
            return;
        }
        Solid::StorageVolume::UsageType type = volume->usage();
        if ((type == Solid::StorageVolume::Unused || type == Solid::StorageVolume::PartitionTable) && !device.is<Solid::OpticalDisc>()) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: device : " << udi << " is not in our interest. Skipping";
            return;
        }
    }

    auto actions = new ActionsControl(udi, this);
    if (!m_encryptedPredicate.matches(device) && actions->isEmpty()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: device : " << udi << " is not in our interest. Skipping";
        actions->deleteLater();
        return;
    }

    m_actions[udi] = actions;
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: New device added : " << udi;

    int position = m_devices.size();

    for (auto type : m_types) {
        const Solid::DeviceInterface *interface = device.asDeviceInterface(type);
        if (interface) {
            m_deviceTypes[udi].first = Solid::DeviceInterface::typeDescription(type);
            break;
        }
    }

    m_deviceTypes[udi].second.first = device.icon();
    m_deviceTypes[udi].second.second = device.description();

    beginInsertRows(QModelIndex(), position, position);

    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Add device: " << udi << " to the model at position : " << position;
    m_stateMonitor->addMonitoringDevice(udi);
    m_spaceMonitor->addMonitoringDevice(udi);
    m_errorMonitor->addMonitoringDevice(udi);
    m_devices.append(device);
    endInsertRows();

    // Save storage drive parent for storage volumes to delay remove it and to properly remove it from device model
    // if device was physically removed from the computer. Storage volume with storage drive parent need to
    // be delay removed to show last message from deviceerrormonitor. Other devices don't have such message
    // so don't need to delay remove them.
    if (m_stateMonitor->isRemovable(udi) && device.is<Solid::StorageVolume>()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Save parent device: " << m_devices[position].parent().udi() << "for device: " << udi;
        if (auto it = m_parentDevices.find(udi); it != m_parentDevices.end()) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Parent already present: append to parent`s list";
            it->append(m_devices[position]);
        } else {
            if (m_devices[position].parent().is<Solid::StorageDrive>()) {
                qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: " << "Creating a new parent list";
                m_parentDevices.insert(m_devices[position].parent().udi(), {m_devices[position]});
            } else {
                qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Parent device is not valid. Don't add one";
            }
        }
    }
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: device: " << udi << " successfully added to the model";
}

void DeviceControl::onDeviceRemoved(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Removed device signal arrived : " << udi;

    // No need to keep device anymore because it was physically removed.
    if (auto it = m_parentDevices.constFind(udi); it != m_parentDevices.cend()) {
        int size = it->size();
        for (int device = 0; device < size; ++device) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Parent was removed for : " << udi;

            deviceDelayRemove(it->at(device).udi(), udi);
        }
        return;
    }

    if (!m_actions.contains(udi)) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Removed device not exist. Skipping : " << udi;
        return;
    }

    for (int position = 0; position < m_devices.size(); ++position) {
        if (m_devices[position].udi() == udi) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Begin remove device: " << udi << " from the model at position : " << position;

            m_actions[udi]->deleteLater();
            m_actions.remove(udi);
            QModelIndex index = DeviceControl::index(position);
            Q_EMIT dataChanged(index, index, {Actions});

            // remove space monitoring because device not mounted
            m_spaceMonitor->removeMonitoringDevice(udi);

            for (auto it = m_parentDevices.begin(); it != m_parentDevices.end(); ++it) {
                for (int position = 0; position < it->size(); ++position) {
                    if (udi == it->at(position).udi()) {
                        const QString &parentUdi = it.key();
                        auto it = m_removeTimers.insert(udi, new QTimer(this));
                        it.value()->setSingleShot(true);
                        it.value()->setInterval(std::chrono::seconds(5));
                        // this keeps the delegate around for 5 seconds after the device has been
                        // removed in case there was a message, such as "you can now safely remove this"
                        connect(it.value(), &QTimer::timeout, this, [this, udi, parentUdi]() {
                            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Timer activated for " << udi;
                            for (int position = 0; position < m_devices.size(); ++position) {
                                if (m_devices[position].udi() == udi) {
                                    deviceDelayRemove(udi, parentUdi);
                                    break;
                                }
                            }
                        });

                        it.value()->start();
                        return;
                    }
                }
            }
            deviceDelayRemove(udi, QString());
        }
    }
}

void DeviceControl::deviceDelayRemove(const QString &udi, const QString &parentUdi)
{
    if (auto it = m_removeTimers.find(udi); it != m_removeTimers.end()) {
        if (it.value()->isActive()) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: device " << udi << " Timer was active: stop";
            it.value()->stop();
        }
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: device " << udi << " Remove timer";
        it.value()->deleteLater();
        m_removeTimers.erase(it);
    }

    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: device " << udi << " : start delay remove";
    if (!parentUdi.isEmpty() && m_stateMonitor->isRemovable(udi)) {
        auto it = m_parentDevices.find(parentUdi);
        for (int position = 0; position < it->size(); ++position) {
            if (udi == it->at(position).udi()) {
                qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: device " << udi << " : found parent device. Removing";
                it->removeAt(position);
                if (it->isEmpty()) {
                    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: parent don't have any child devices. Erase parent";
                    m_parentDevices.erase(it);
                }
                break;
            }
        }
    }

    for (int position = 0; position < m_devices.size(); ++position) {
        if (m_devices[position].udi() == udi) {
            beginRemoveRows(QModelIndex(), position, position);
            if (auto it = m_deviceTypes.find(udi); it != m_deviceTypes.end()) {
                m_deviceTypes.remove(m_devices[position].udi());
            }

            m_stateMonitor->removeMonitoringDevice(m_devices[position].udi());
            m_errorMonitor->removeMonitoringDevice(m_devices[position].udi());

            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: device: " << m_devices[position].udi() << " successfully removed from the model";
            m_devices.removeAt(position);
            endRemoveRows();
            break;
        }
    }
}

void DeviceControl::onDeviceChanged(const QMap<QString, int> &props)
{
    auto iface = qobject_cast<Solid::GenericInterface *>(sender());
    if (iface && iface->isValid() && props.contains(QLatin1String("Size")) && iface->property(QStringLiteral("Size")).toInt() > 0) {
        const QString udi = qobject_cast<QObject *>(iface)->property("udi").toString();
        m_spaceMonitor->forceUpdateSize(udi);
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: 2-stage device successfully initialized : " << udi;
    }
}

void DeviceControl::onDeviceSizeChanged(const QString &udi)
{
    // update the volume in case of 2-stage devices
    Solid::Device device(udi);
    if (device.is<Solid::StorageVolume>()) {
        bool isDeviceValid = false;

        for (const auto &findingDevice : m_devices) {
            if (findingDevice.udi() == udi) {
                isDeviceValid = true;
            }
        }

        if (isDeviceValid && m_spaceMonitor->getFullSize(udi) == 0) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: 2-stage device arrived : " << udi;
            Solid::GenericInterface *iface = device.as<Solid::GenericInterface>();
            if (iface) {
                iface->setProperty("udi", device.udi());
                connect(iface, &Solid::GenericInterface::propertyChanged, this, &DeviceControl::onDeviceChanged);
                return;
            }
        }
    }

    for (int position = 0; position < m_devices.size(); ++position) {
        if (m_devices[position].udi() == udi) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Size for device : " << udi << " changed";
            QModelIndex index = DeviceControl::index(position);
            Q_EMIT dataChanged(index, index, {Size, SizeText, FreeSpace, FreeSpaceText});
            return;
        }
    }
}

void DeviceControl::onDeviceStatusChanged(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Status for device : " << udi << " changed";
    for (int position = 0; position < m_devices.size(); ++position) {
        if (m_devices[position].udi() == udi) {
            QModelIndex index = DeviceControl::index(position);
            Q_EMIT dataChanged(index, index, {Mounted, OperationResult, Emblems});
            return;
        }
    }
}

void DeviceControl::onDeviceErrorChanged(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Error for device : " << udi << " changed";
    for (int position = 0; position < m_devices.size(); ++position) {
        if (m_devices[position].udi() == udi) {
            QModelIndex index = DeviceControl::index(position);
            Q_EMIT dataChanged(index, index, {Error, ErrorMessage});
            return;
        }
    }
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Error for device : " << udi << " Fail to update. Device not exists";
}

#include "moc_devicecontrol.cpp"
