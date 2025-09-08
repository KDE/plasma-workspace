/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "devicefiltercontrol.h"

#include <devicenotifier_debug.h>

#include "devicecontrol.h"
#include "devicestatemonitor_p.h"

#include <QDBusConnection>
#include <QDBusMessage>

#include <Solid/Device>
#include <Solid/OpticalDrive>

using namespace Qt::Literals::StringLiterals;

DeviceFilterControl::DeviceFilterControl(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_filterType(Removable)
    , m_isVisible(false)
    , m_modelReset(false)
    , m_spaceMonitor(SpaceMonitor::instance())
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Begin initializing Device Filter Control";
    setSourceModel(new DeviceControl(this));
    setDynamicSortFilter(false);

    onModelReset();

    connect(this, &DeviceFilterControl::rowsInserted, this, &DeviceFilterControl::onDeviceAdded);
    connect(this, &DeviceFilterControl::rowsAboutToBeRemoved, this, &DeviceFilterControl::onDeviceRemoved);
    connect(this, &DeviceFilterControl::modelReset, this, &DeviceFilterControl::onModelReset);

    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control successfully initialized";
}

DeviceFilterControl::~DeviceFilterControl() = default;

void DeviceFilterControl::unmountAllRemovables()
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: unmount all removables function invoked";
    for (int position = 0; position < rowCount(); ++position) {
        auto index = DeviceFilterControl::index(position, 0);
        auto actionData = data(index, DeviceControl::Actions);
        if (!actionData.isNull()) {
            auto actions = qvariant_cast<ActionsControl *>(actionData);
            if (actions->isUnmountable()) {
                actions->unmount();
            }
        }
    }
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: unmount all removables function finished";
}

void DeviceFilterControl::dismissUsbDeviceAddedNotification()
{
    QDBusMessage msg = QDBusMessage::createMethodCall(u"org.kde.kded6"_s,
                                                      u"/modules/devicenotifications"_s,
                                                      u"org.kde.plasma.devicenotifications"_s,
                                                      u"dismissUsbDeviceAdded"_s);
    QDBusConnection::sessionBus().call(msg, QDBus::NoBlock);
}

QBindable<QString> DeviceFilterControl::bindableLastUdi()
{
    return &m_lastUdi;
}

QBindable<QString> DeviceFilterControl::bindableLastDescription()
{
    return &m_lastDescription;
}

QBindable<QString> DeviceFilterControl::bindableLastIcon()
{
    return &m_lastIcon;
}

QBindable<bool> DeviceFilterControl::bindableLastDeviceAdded()
{
    return &m_lastDeviceAdded;
}

QBindable<qsizetype> DeviceFilterControl::bindableDeviceCount()
{
    return &m_deviceCount;
}

QBindable<qsizetype> DeviceFilterControl::bindableUnmountableCount()
{
    return &m_unmountableCount;
}

DeviceFilterControl::DevicesType DeviceFilterControl::filterType() const
{
    return m_filterType;
}

void DeviceFilterControl::setFilterType(DevicesType type)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: filter type: "
                                     << (type == Removable         ? "Removable"
                                             : type == Unremovable ? "Not removable"
                                                                   : "All");
    if (type != m_filterType) {
        m_filterType = type;
        m_modelReset = true;
        invalidateRowsFilter();
        onModelReset();
        m_modelReset = false;
    }
}

bool DeviceFilterControl::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    if (!index.isValid()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: index is invalid";
        return false;
    }

    switch (m_filterType) {
    case All:
        return true;
    case Removable:
        return sourceModel()->data(index, DeviceControl::IsRemovable).toBool();
    case Unremovable:
        return !sourceModel()->data(index, DeviceControl::IsRemovable).toBool();
    }

    return false;
}
bool DeviceFilterControl::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    if (!source_left.isValid()) {
        return true;
    }

    if (!source_right.isValid()) {
        return false;
    }

    QString leftType = sourceModel()->data(source_left, DeviceControl::Type).toString();
    QString rightType = sourceModel()->data(source_right, DeviceControl::Type).toString();

    if (leftType < rightType) {
        return true;
    }

    if (leftType > rightType) {
        return false;
    }

    QDateTime leftTime = sourceModel()->data(source_left, DeviceControl::Timestamp).toDateTime();
    QDateTime rightTime = sourceModel()->data(source_right, DeviceControl::Timestamp).toDateTime();

    if (leftTime < rightTime) {
        return false;
    }

    return true;
}

bool DeviceFilterControl::isVisible() const
{
    return m_isVisible;
}

void DeviceFilterControl::setIsVisible(bool status)
{
    m_isVisible = status;
    m_spaceMonitor->setIsVisible(status);
}

void DeviceFilterControl::onDeviceAdded(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(last);

    if (m_modelReset) {
        return;
    }

    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: rowInserted signal arrived";

    m_deviceCount = rowCount(parent);

    m_lastDeviceAdded = true;
    QModelIndex index = DeviceFilterControl::index(first, 0, parent);

    handleDeviceAdded(index);

    sort(0, Qt::AscendingOrder);
}

void DeviceFilterControl::onDeviceRemoved(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(last);

    if (m_modelReset) {
        return;
    }

    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: rowRemoved signal arrived";

    m_deviceCount = rowCount() - 1;

    QModelIndex index = DeviceFilterControl::index(first, 0, parent);

    if (m_filterType != Unremovable) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: filter type is not Unremovable. updating unmountAll Action";

        if (auto it = m_unmountableDevices.constFind(data(index, DeviceControl::Udi).toString()); it != m_unmountableDevices.constEnd()) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: remove device " << data(index, DeviceControl::Udi).toString()
                                             << " from unmountable devices";
            m_unmountableDevices.erase(it);
        } else {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: device " << data(index, DeviceControl::Udi).toString()
                                             << "device is not unmountable. Skipping";
        }
    }

    m_unmountableCount = m_unmountableDevices.count();
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: Unmountable count updated: " << m_unmountableCount.value();

    if (m_lastUdi.value() == data(index, DeviceControl::Udi).toString()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: device " << data(index, DeviceControl::Udi).toString()
                                         << "was last added device. Set new last device";

        if (!m_deviceOrder.isEmpty()) {
            m_lastDeviceAdded = false;
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: device " << m_lastUdi.value() << "become new last device";
            Solid::Device device = Solid::Device(m_deviceOrder.top());
            m_lastIcon = device.icon();
            m_lastDescription = device.description();
            m_lastUdi = m_deviceOrder.pop();
        } else {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: no last devices found. Clear last device";
            m_lastDeviceAdded = false;
            m_lastIcon = QString();
            m_lastDescription = QString();
            m_lastUdi = QString();
        }
    } else {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: device " << data(index, DeviceControl::Udi).toString()
                                         << "is not last. Begin removing from device order";
        for (int position = 0; position < m_deviceOrder.size(); ++position) {
            if (m_deviceOrder[position] == data(index, DeviceControl::Udi).toString()) {
                m_deviceOrder.removeAt(position);
                qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: device " << data(index, DeviceControl::Udi).toString() << "removed at position "
                                                 << position;
                break;
            }
        }
    }

    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: device " << data(index, DeviceControl::Udi).toString() << "successfully removed";
}

void DeviceFilterControl::onModelReset()
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: modelResetSignal arrived. Begin resetting model";

    m_deviceOrder.clear();
    m_unmountableDevices.clear();

    m_deviceCount = rowCount();

    m_lastDeviceAdded = false;
    m_lastIcon = QString();
    m_lastDescription = QString();
    m_lastUdi = QString();

    for (int modelPosition = 0; modelPosition < rowCount(); ++modelPosition) {
        handleDeviceAdded(DeviceFilterControl::index(modelPosition, 0));
    }

    sort(0, Qt::AscendingOrder);

    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: modelResetSignal arrived. Resetting model finished";
}

void DeviceFilterControl::onDeviceActionUnmountableChanged(const QString &udi, bool status)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: DeviceActionUnmountable arrived for device" << udi;
    if (status) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: device " << udi << "added to unmountable devices";
        m_unmountableDevices.insert(udi);
    } else {
        if (auto it = m_unmountableDevices.constFind(udi); it != m_unmountableDevices.constEnd()) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: device " << udi << "removed from unmountable devices";
            m_unmountableDevices.erase(it);
        }
    }

    m_unmountableCount = m_unmountableDevices.count();

    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: Unmountable count updated: " << m_unmountableCount.value();
}

void DeviceFilterControl::handleDeviceAdded(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    if (!m_lastUdi.value().isEmpty()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: save last udi " << m_lastUdi.value();
        m_deviceOrder.push(m_lastUdi);
    } else {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: no last udi present. Skipping";
    }

    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: Set new last Device " << data(index, DeviceControl::Udi).toString();

    m_lastIcon = data(index, DeviceControl::Icon).toString();
    m_lastDescription = data(index, DeviceControl::Description).toString();
    m_lastUdi = data(index, DeviceControl::Udi).toString();

    if (m_filterType != Unremovable) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: filter type is not Unremovable. updating unmountAll Action";
        auto actionData = data(index, DeviceControl::Actions);
        if (!actionData.isNull()) {
            auto actions = qvariant_cast<ActionsControl *>(actionData);
            connect(actions, &ActionsControl::unmountActionIsValidChanged, this, &DeviceFilterControl::onDeviceActionUnmountableChanged);
            if (actions->isUnmountable()) {
                qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: add device " << data(index, DeviceControl::Udi).toString()
                                                 << " to unmountable devices";
                m_unmountableDevices.insert(data(index, DeviceControl::Udi).toString());
            } else {
                qCDebug(APPLETS::DEVICENOTIFIER) << "Device Filter Control: device " << data(index, DeviceControl::Udi).toString()
                                                 << "device is not unmountable. Skipping";
            }
        }
    }

    m_unmountableCount = m_unmountableDevices.count();
}

#include "moc_devicefiltercontrol.cpp"
