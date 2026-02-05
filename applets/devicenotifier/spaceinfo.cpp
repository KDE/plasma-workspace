/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "spaceinfo.h"

#include "devicenotifier_debug.h"

#include <Solid/Device>
#include <Solid/DeviceInterface>
#include <Solid/GenericInterface>
#include <Solid/StorageAccess>
#include <Solid/StorageVolume>

#include <KIO/FileSystemFreeSpaceJob>

#include <QTimer>
#include <QUrl>

#include "spaceupdatemonitor_p.h"

SpaceInfo::SpaceInfo(const std::shared_ptr<StorageInfo> &storageInfo, const std::shared_ptr<StateInfo> &stateInfo, QObject *parent)
    : QObject(parent)
    , m_storageInfo(storageInfo)
    , m_stateInfo(stateInfo)
    , m_spaceUpdateMonitor(SpaceUpdateMonitor::instance())
{
    connect(m_spaceUpdateMonitor.get(), &SpaceUpdateMonitor::updateSpace, this, &SpaceInfo::updateSize);
    connect(m_stateInfo.get(), &StateInfo::stateChanged, this, &SpaceInfo::onStateChanged);

    updateSize();

    qCDebug(APPLETS::DEVICENOTIFIER) << "Space Info " << m_storageInfo->device().udi() << " : created";
}

SpaceInfo::~SpaceInfo()
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Space Info " << m_storageInfo->device().udi() << " : removed";
}

std::optional<double> SpaceInfo::getFullSize() const
{
    return m_fullSize;
}

std::optional<double> SpaceInfo::getFreeSize() const
{
    return m_freeSize;
}

void SpaceInfo::onStateChanged()
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Space Info " << m_storageInfo->device().udi() << " : device state changed! Force updating space";
    updateSize();
}

void SpaceInfo::updateSize()
{
    const Solid::Device &device = m_storageInfo->device();
    auto storageaccess = device.as<Solid::StorageAccess>();
    if (!storageaccess || !m_stateInfo->isMounted()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Space Info " << device.udi() << " : failed to get storage access";
        m_freeSize = std::nullopt;
        m_fullSize = std::nullopt;
        Q_EMIT sizeChanged(device.udi());
        return;
    }

    QString path = storageaccess->filePath();

    // create job
    m_job = KIO::fileSystemFreeSpace(QUrl::fromLocalFile(path));

    // collect and process info
    connect(m_job, &KJob::result, this, &SpaceInfo::onSpaceUpdated);
}

void SpaceInfo::onSpaceUpdated(KJob *job)
{
    const QString &udi = m_storageInfo->device().udi();

    if (!job->error() && m_job) {
        // update the volume in case of 2-stage devices
        Solid::Device device = m_storageInfo->device();
        if (device.is<Solid::StorageVolume>()) {
            if (m_job->size() == 0) {
                qCDebug(APPLETS::DEVICENOTIFIER) << "Space Info " << device.udi() << " : 2-stage device arrived : " << udi;
                auto iface = device.as<Solid::GenericInterface>();
                if (iface) {
                    iface->setProperty("udi", device.udi());
                    connect(iface, &Solid::GenericInterface::propertyChanged, this, &SpaceInfo::onDeviceChanged);
                    return;
                }
            }
        }

        m_fullSize = m_job->size();
        m_freeSize = m_job->availableSize();

        qCDebug(APPLETS::DEVICENOTIFIER) << "Space Info " << udi << " : storage space update finished. "
                                         << "Space: " << m_fullSize << "FreeSpace: " << m_freeSize;
    } else {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Space Info " << udi << " : Failed to get size";
        m_fullSize = std::nullopt;
        m_freeSize = std::nullopt;
    }
    Q_EMIT sizeChanged(udi);
}

void SpaceInfo::onDeviceChanged(const QMap<QString, int> &props)
{
    auto iface = qobject_cast<Solid::GenericInterface *>(sender());
    if (iface && iface->isValid() && props.contains(QLatin1String("Size")) && iface->property(QStringLiteral("Size")).toInt() > 0) {
        const QString udi = qobject_cast<QObject *>(iface)->property("udi").toString();
        updateSize();
        qCDebug(APPLETS::DEVICENOTIFIER) << "Space Info " << udi << " : 2-stage device successfully initialized";
    }
}

#include "moc_spaceinfo.cpp"
