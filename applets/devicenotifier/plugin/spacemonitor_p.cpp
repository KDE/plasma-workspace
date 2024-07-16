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

#include <KLocalizedString>
#include <KNotification>

SpaceMonitor::SpaceMonitor(QObject *parent)
    : m_spaceWatcher(new QTimer(this))
    , QObject(parent)
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
    if (!m_sizes.contains(udi)) {
        return -1;
    }
    return m_sizes[udi].first;
}

double SpaceMonitor::getFreeSize(const QString &udi) const
{
    if (!m_sizes.contains(udi)) {
        return -1;
    }
    return m_sizes[udi].second;
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
    qCDebug(APPLETS::DEVICENOTIFIER) << "Space Monitor: remove device " << udi;
    m_sizes.remove(udi);
}

void SpaceMonitor::forceUpdateSize(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Space Monitor: forced to update size for device  " << udi;
    updateStorageSpace(udi);
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

    Solid::StorageAccess *storageaccess = device.as<Solid::StorageAccess>();
    if (!storageaccess || !storageaccess->isAccessible()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Space Monitor: failed to get storage access " << udi;
        m_sizes[udi].first = -1;
        m_sizes[udi].second = -1;
        Q_EMIT sizeChanged(udi);
        return;
    }

    QString path = storageaccess->filePath();
    QTimer *timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, [path, udi]() {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Space Monitor: timeout when updating filesystem size for device " << udi << ". Not responding";
        KNotification::event(KNotification::Error, i18n("Filesystem is not responding"), i18n("Filesystem mounted at '%1' is not responding", path));
    });

    // create job
    KIO::FileSystemFreeSpaceJob *job = KIO::fileSystemFreeSpace(QUrl::fromLocalFile(path));

    // delete later timer
    connect(job, &KJob::result, timer, &QTimer::deleteLater);

    // collect and process info
    connect(job, &KJob::result, this, [this, timer, udi, job]() {
        timer->stop();

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

    // start timer: after 15 seconds we will get an error
    timer->start(std::chrono::seconds(15));
}

#include "moc_spacemonitor_p.cpp"
