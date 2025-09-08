/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "spacemonitor_p.h"

#include "devicenotifier_debug.h"

#include <Solid/Device>
#include <Solid/DeviceInterface>
#include <Solid/StorageAccess>

#include <KIO/FileSystemFreeSpaceJob>

#include <QTimer>
#include <QUrl>

SpaceMonitor::SpaceMonitor(QObject *parent)
    : QObject(parent)
    , m_spaceWatcher(new QTimer(this))
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Begin initializing Space Monitor";
    m_spaceWatcher->setSingleShot(true);
    m_spaceWatcher->setInterval(std::chrono::minutes(1));
    connect(m_spaceWatcher, &QTimer::timeout, this, &SpaceMonitor::updateAllStorageSpaces);
    m_stateMonitor = DevicesStateMonitor::instance();
    connect(m_stateMonitor.get(), &DevicesStateMonitor::stateChanged, this, &SpaceMonitor::deviceStateChanged);
    qCDebug(APPLETS::DEVICENOTIFIER) << "Space Monitor initialized";
}

SpaceMonitor::~SpaceMonitor()
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Space Monitor was removed";
    m_spaceWatcher->stop();
}

std::shared_ptr<SpaceMonitor> SpaceMonitor::instance()
{
    static std::weak_ptr<SpaceMonitor> s_clip;
    if (s_clip.expired()) {
        std::shared_ptr<SpaceMonitor> ptr{new SpaceMonitor};
        s_clip = ptr;
        return ptr;
    }
    return s_clip.lock();
}

double SpaceMonitor::getFullSize(const QString &udi) const
{
    if (auto it = m_sizes.constFind(udi); it != m_sizes.constEnd()) {
        return it->first;
    }
    return -1;
}

double SpaceMonitor::getFreeSize(const QString &udi) const
{
    if (auto it = m_sizes.constFind(udi); it != m_sizes.constEnd()) {
        return it->second;
    }
    return -1;
}

void SpaceMonitor::setIsVisible(bool status)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Space Monitor: is Visible changed to " << status;
    if (status) {
        m_spaceWatcher->setSingleShot(false);
        if (!m_spaceWatcher->isActive()) {
            updateAllStorageSpaces();
            m_spaceWatcher->start();
        }
    } else {
        m_spaceWatcher->setSingleShot(true);
    }
}

void SpaceMonitor::addMonitoringDevice(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Space Monitor: Adding new device " << udi;
    updateStorageSpace(udi);
}

void SpaceMonitor::removeMonitoringDevice(const QString &udi)
{
    if (auto it = m_sizes.find(udi); it != m_sizes.end()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Space Monitor: remove device " << udi;
        m_sizes.remove(udi);
        Q_EMIT sizeChanged(udi);
    } else {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Space Monitor: device " << udi << " not found";
    }
}

void SpaceMonitor::forceUpdateSize(const QString &udi)
{
    if (auto it = m_sizes.find(udi); it != m_sizes.end()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Space Monitor: forced to update size for device  " << udi;
        updateStorageSpace(udi);
    } else {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Space Monitor: device " << udi << " not found";
    }
}

void SpaceMonitor::deviceStateChanged(QString udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Space Monitor: device state changed! Force updating space";
    updateStorageSpace(udi);
}

void SpaceMonitor::updateAllStorageSpaces()
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Space Monitor: Timer is out. Begin updating all storages status ";
    if (m_sizes.isEmpty()) {
        return;
    }

    QHashIterator it(m_sizes);

    while (it.hasNext()) {
        it.next();
        updateStorageSpace(it.key());
    }
}

void SpaceMonitor::updateStorageSpace(const QString &udi)
{
    Solid::Device device(udi);

    auto *storageaccess = device.as<Solid::StorageAccess>();
    if (!storageaccess || !storageaccess->isAccessible()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Space Monitor: failed to get storage access " << udi;
        m_sizes[udi].first = -1;
        m_sizes[udi].second = -1;
        Q_EMIT sizeChanged(udi);
        return;
    }

    QString path = storageaccess->filePath();

    // create job
    KIO::FileSystemFreeSpaceJob *job = KIO::fileSystemFreeSpace(QUrl::fromLocalFile(path));

    // collect and process info
    connect(job, &KJob::result, this, [this, udi, job]() {
        if (!job->error()) {
            KIO::filesize_t size = job->size();
            KIO::filesize_t available = job->availableSize();

            m_sizes[udi] = {size, available};
            qCDebug(APPLETS::DEVICENOTIFIER) << "Space Monitor: storage space update finished for " << udi << "Space: " << size << "FreeSpace: " << available;
            Q_EMIT sizeChanged(udi);
        } else {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Space Monitor: Failed to get size for : " << udi;
        }
    });
}

#include "moc_spacemonitor_p.cpp"
