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

using namespace std::chrono_literals;

inline constexpr auto REMOVE_INTERVAL = 5s;

DeviceControl::DeviceControl(QObject *parent)
    : QAbstractListModel(parent)
    , m_spaceMonitor(SpaceMonitor::instance())
    , m_messageMonitor(DeviceMessageMonitor::instance())

{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Begin initializing";

    auto devices = Solid::Device::listFromQuery(StorageInfo::predicate());
    for (const auto &device : devices) {
        onDeviceAdded(device.udi());
    }

    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceAdded, this, &DeviceControl::onDeviceAdded);
    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceRemoved, this, &DeviceControl::onDeviceRemoved);

    connect(m_spaceMonitor.get(), &SpaceMonitor::sizeChanged, this, &DeviceControl::onDeviceSizeChanged);
    connect(m_messageMonitor.get(), &DeviceMessageMonitor::messageChanged, this, &DeviceControl::onDeviceMessageChanged);
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Initialized";
}

DeviceControl::~DeviceControl() = default;

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

    auto &deviceInfo = m_devices.at(index.row());
    
    if (!deviceInfo.storageInfo) {
        return {};
    }

    switch (role) {
    case Udi:
        return deviceInfo.storageInfo ? deviceInfo.storageInfo->device().udi() : QVariant();
    case Icon:
        return deviceInfo.storageInfo ? deviceInfo.storageInfo->icon() : QVariant();
    case Emblems:
        return deviceInfo.storageInfo ? deviceInfo.storageInfo->device().emblems() : QVariant();
    case Description:
        return deviceInfo.storageInfo ? deviceInfo.storageInfo->description() : QVariant();
    case IsBusy:
        return deviceInfo.stateInfo ? deviceInfo.stateInfo->isBusy() : QVariant();
    case IsRemovable: {
        return deviceInfo.storageInfo ? deviceInfo.storageInfo->isRemovable() : QVariant();
    }
    case Size:
        return m_spaceMonitor->getFullSize(deviceInfo.storageInfo->device().udi());
    case FreeSpace:
        return m_spaceMonitor->getFreeSize(deviceInfo.storageInfo->device().udi());
    case SizeText: {
        double size = m_spaceMonitor->getFullSize(deviceInfo.storageInfo->device().udi());
        return size != -1 ? KFormat().formatByteSize(size) : QString();
    }
    case FreeSpaceText: {
        double freeSpace = m_spaceMonitor->getFreeSize(deviceInfo.storageInfo->device().udi());
        return freeSpace != -1 ? KFormat().formatByteSize(freeSpace) : QString();
    }
    case Mounted:
        return deviceInfo.stateInfo ? deviceInfo.stateInfo->isMounted() : QVariant();
    case State:
        return deviceInfo.stateInfo ? deviceInfo.stateInfo->getState() : QVariant();
    case Timestamp: {
        return deviceInfo.stateInfo ? deviceInfo.stateInfo->getDeviceTimeStamp() : QVariant();
    }
    case Type: {
        return deviceInfo.storageInfo ? deviceInfo.storageInfo->type() : QVariant();
    }
    case OperationResult:
        return deviceInfo.stateInfo ? deviceInfo.stateInfo->getOperationResult() : QVariant();
    case Message:
        return m_messageMonitor->getMessage(deviceInfo.storageInfo->device().udi());
    case Actions: {
        if (auto it = m_actions.constFind(deviceInfo.storageInfo->device().udi()); it != m_actions.end()) {
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
    roles[IsBusy] = "deviceIsBusy";
    roles[IsRemovable] = "deviceIsRemovable";
    roles[FreeSpace] = "deviceFreeSpace";
    roles[Size] = "deviceSize";
    roles[FreeSpaceText] = "deviceFreeSpaceText";
    roles[SizeText] = "deviceSizeText";
    roles[Mounted] = "deviceMounted";
    roles[State] = "deviceState";
    roles[OperationResult] = "deviceOperationResult";
    roles[Timestamp] = "deviceTimestamp";
    roles[Message] = "deviceMessage";
    roles[Actions] = "deviceActions";
    return roles;
}

void DeviceControl::onDeviceAdded(const QString &udi)
{
    // There is a possibility that a device already present. Check it.
    if (m_devicesUdi.contains(udi)) {
        return;
    }

    auto storageInfo = std::make_shared<StorageInfo>(udi);

    // check if the device is a storage device
    if (!storageInfo->isValid()) {
        return;
    }

    auto stateInfo = std::make_shared<StateInfo>(storageInfo);

    auto actions = new ActionsControl(storageInfo, stateInfo, this);

    if (!storageInfo->isEncrypted() && actions->isEmpty()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: device : " << udi << " is not in our interest. Skipping";
        actions->deleteLater();
        return;
    }

    if (auto it = m_removeTimers.constFind(udi); it != m_removeTimers.cend()) {
        deviceDelayRemove(it->udi, it->parentUdi); // A device is removed and added back immediately, can happen during formatting
    }

    m_actions[udi] = actions;
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: New device added : " << udi;

    int position = m_devices.size();

    beginInsertRows(QModelIndex(), position, position);

    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Add device: " << udi << " to the model at position : " << position;
    m_spaceMonitor->addMonitoringDevice(udi, stateInfo);
    m_messageMonitor->addMonitoringDevice(udi, stateInfo);

    DeviceInfo deviceInfo{storageInfo, stateInfo};

    m_devices.append(deviceInfo);
    m_devicesUdi.insert(udi);
    endInsertRows();

    connect(stateInfo.get(), &StateInfo::stateChanged, this, &DeviceControl::onDeviceStatusChanged);

    // Save storage drive parent for storage volumes to delay remove it and to properly remove it from device model
    // if device was physically removed from the computer. Storage volume with storage drive parent need to
    // be delay removed to show last message from deviceerrormonitor. Other devices don't have such message
    // so don't need to delay remove them.
    if (deviceInfo.storageInfo->hasRemovableParent()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Save parent device: " << deviceInfo.storageInfo->device().parent().udi()
                                         << "for device: " << udi;
        if (auto it = m_parentDevices.find(udi); it != m_parentDevices.end()) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Parent already present: append to parent`s list";
            it->append(deviceInfo.storageInfo);
        } else {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Creating a new parent list";
            m_parentDevices.insert(deviceInfo.storageInfo->device().parent().udi(), {deviceInfo.storageInfo});
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
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Parent was removed for : " << udi;
        for (int device = 0; device < size; ++device) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Remove child : " << it->at(device)->device().udi();
            deviceDelayRemove(it->at(device)->device().udi(), udi);
        }
        return;
    }

    if (!m_devicesUdi.contains(udi)) {
        return;
    }

    for (int position = 0; position < m_devices.size(); ++position) {
        if (m_devices[position].storageInfo->device().udi() == udi) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Begin remove device: " << udi << " from the model at position : " << position;

            ActionsControl *actions = m_actions.take(udi);
            QModelIndex index = DeviceControl::index(position);
            Q_EMIT dataChanged(index, index, {Actions});
            delete actions;

            // remove space monitoring because device not mounted
            m_spaceMonitor->removeMonitoringDevice(udi);

            for (auto it = m_parentDevices.begin(); it != m_parentDevices.end(); ++it) {
                for (int childPosition = 0; childPosition < it->size(); ++childPosition) {
                    if (udi == it->at(childPosition)->device().udi()) {
                        auto timer = std::make_shared<QTimer>();
                        timer->setSingleShot(true);
                        timer->setInterval(REMOVE_INTERVAL);
                        // this keeps the delegate around for 5 seconds after the device has been
                        // removed in case there was a message, such as "you can now safely remove this"
                        connect(timer.get(), &QTimer::timeout, this, [this, udi] {
                            const RemoveTimerData &data = m_removeTimers[udi];
                            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Timer activated for " << udi;
                            Q_ASSERT(udi == data.udi);
                            deviceDelayRemove(udi, data.parentUdi);
                        });
                        timer->start();
                        m_removeTimers.insert(udi, {timer, udi, it.key()});
                        return;
                    }
                }
            }
            deviceDelayRemove(udi, QString());
            break;
        }
    }
}

void DeviceControl::deviceDelayRemove(const QString &udi, const QString &parentUdi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: device " << udi << " : start delay remove";

    std::optional<int> position = std::nullopt;

    for (int findPosition = 0; findPosition < m_devices.size(); ++findPosition) {
        if (m_devices[findPosition].storageInfo->device().udi() == udi) {
            position = findPosition;
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: device is at position : " << position;
            break;
        }
    }

    if (!position.has_value()) {
        qCWarning(APPLETS::DEVICENOTIFIER) << "Device Controller: device is not found";
        return;
    }

    if (!parentUdi.isEmpty() && m_devices[position.value()].storageInfo->isRemovable()) {
        auto it = m_parentDevices.find(parentUdi);
        if (it != m_parentDevices.end()) { // PLASMA-WORKSPACE-146Y: If a parent device is not of StorageDrive type
            for (int childPosition = 0; childPosition < it->size(); ++childPosition) {
                if (udi == it->at(childPosition)->device().udi()) {
                    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: device " << udi << " : found parent device. Removing";
                    it->removeAt(childPosition);
                    if (it->isEmpty()) {
                        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: parent don't have any child devices. Erase parent";
                        m_parentDevices.erase(it);
                    }
                    break;
                }
            }
        }
    }

    beginRemoveRows(QModelIndex(), position.value(), position.value());
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: device: " << m_devices[position.value()].storageInfo->device().udi()
                                     << " successfully removed from the model";
    m_devices.removeAt(position.value());
    m_devicesUdi.remove(udi);
    m_messageMonitor->removeMonitoringDevice(udi);
    endRemoveRows();

    if (auto it = m_removeTimers.find(udi); it != m_removeTimers.end()) {
        if (it->timer->isActive()) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: device " << udi << " Timer was active: stop";
            it->timer->stop();
        }
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: device " << udi << " Remove timer";
        m_removeTimers.erase(it); // PLASMA-WORKSPACE-15SV: This must be removed at the end to ensure udi and parentUdi are not dangling
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
            if (findingDevice.storageInfo && findingDevice.storageInfo->device().udi() == udi) {
                isDeviceValid = true;
            }
        }

        if (isDeviceValid && m_spaceMonitor->getFullSize(udi) == 0) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: 2-stage device arrived : " << udi;
            auto *iface = device.as<Solid::GenericInterface>();
            if (iface) {
                iface->setProperty("udi", device.udi());
                connect(iface, &Solid::GenericInterface::propertyChanged, this, &DeviceControl::onDeviceChanged);
                return;
            }
        }
    }

    for (int position = 0; position < m_devices.size(); ++position) {
        if (m_devices[position].storageInfo && m_devices[position].storageInfo->device().udi() == udi) {
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
        if (m_devices[position].storageInfo && m_devices[position].storageInfo->device().udi() == udi) {
            QModelIndex index = DeviceControl::index(position);
            Q_EMIT dataChanged(index, index, {Mounted, State, OperationResult, Emblems, IsBusy});
            return;
        }
    }
}

void DeviceControl::onDeviceMessageChanged(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Error for device : " << udi << " changed";
    for (int position = 0; position < m_devices.size(); ++position) {
        if (m_devices[position].storageInfo && m_devices[position].storageInfo->device().udi() == udi) {
            QModelIndex index = DeviceControl::index(position);
            Q_EMIT dataChanged(index, index, {Message});
            return;
        }
    }
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Controller: Error for device : " << udi << " Fail to update. Device not exists";
}

#include "moc_devicecontrol.cpp"
