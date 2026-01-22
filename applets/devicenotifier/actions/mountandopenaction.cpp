/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mountandopenaction.h"

#include <Solid/Camera>
#include <Solid/OpticalDisc>
#include <Solid/OpticalDrive>
#include <Solid/PortableMediaPlayer>

#include <KLocalizedString>

#include <devicenotifier_debug.h>

MountAndOpenAction::MountAndOpenAction(const std::shared_ptr<StorageInfo> &storageInfo, QObject *parent)
    : ActionInterface(storageInfo, parent)
    , m_stateMonitor(DevicesStateMonitor::instance())
{
    const Solid::Device &device = m_storageInfo->device();

    m_hasStorageAccess = false;
    m_isOpticalDisk = false;
    m_isRoot = false;

    m_hasPortableMediaPlayer = false;
    m_hasCamera = false;

    if (device.is<Solid::StorageAccess>()) {
        auto *storageaccess = device.as<Solid::StorageAccess>();
        if (storageaccess) {
            m_hasStorageAccess = true;
            m_isRoot = storageaccess->filePath() == u"/";
        }

        if (device.is<Solid::OpticalDisc>()) {
            auto *opticaldisc = device.as<Solid::OpticalDisc>();
            if (opticaldisc) {
                m_isOpticalDisk = true;
            }
        }
    }

    if (device.is<Solid::PortableMediaPlayer>()) {
        auto *mediaplayer = device.as<Solid::PortableMediaPlayer>();
        if (mediaplayer) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "MountAndOpenAction: Device " << m_storageInfo->device().udi() << " has a media player";
            m_hasPortableMediaPlayer = true;
            m_supportedProtocols.append(mediaplayer->supportedProtocols());
            qCDebug(APPLETS::DEVICENOTIFIER) << "MountAndOpenAction: Supported protocols: " << m_supportedProtocols;
        }
    }

    if (device.is<Solid::Camera>()) {
        auto *camera = device.as<Solid::Camera>();
        if (camera) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "MountAndOpenAction: Device " << m_storageInfo->device().udi() << " has a camera";
            m_hasCamera = true;
            m_supportedProtocols.append(camera->supportedProtocols());
            qCDebug(APPLETS::DEVICENOTIFIER) << "MountAndOpenAction: Supported protocols: " << m_supportedProtocols;
        }
    }

    connect(m_stateMonitor.get(), &DevicesStateMonitor::stateChanged, this, &MountAndOpenAction::updateAction);

    updateAction(m_storageInfo->device().udi());
}

MountAndOpenAction::~MountAndOpenAction() = default;

QString MountAndOpenAction::predicate() const
{
    QString newPredicate;

    if (!m_hasStorageAccess || !m_storageInfo->isRemovable() || !m_stateMonitor->isMounted(m_storageInfo->device().udi())) {
        newPredicate = QLatin1String("openWithFileManager.desktop");

        if (!m_hasStorageAccess && (m_hasPortableMediaPlayer || m_hasCamera)) {
            if (m_supportedProtocols.isEmpty()) {
                return newPredicate;
            }

            for (const QString &protocol : m_supportedProtocols) {
                if (protocol == u"mtp") {
                    newPredicate = QLatin1String("solid_mtp.desktop"); // this lives in kio-extras!
                    break;
                }
                if (protocol == u"afc") {
                    newPredicate = QLatin1String("solid_afc.desktop"); // this lives in kio-extras!
                    break;
                }
                if (protocol == u"ptp") {
                    newPredicate = QLatin1String("solid_camera.desktop");
                    break;
                }
            }
        }
    }
    return newPredicate;
}

bool MountAndOpenAction::isValid() const
{
    return true;
}

QString MountAndOpenAction::name() const
{
    return QStringLiteral("MountAndOpen");
}

QString MountAndOpenAction::icon() const
{
    return m_icon;
}

QString MountAndOpenAction::text() const
{
    return m_text;
}

void MountAndOpenAction::triggered()
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Mount And Open action triggered";

    Solid::Device device = m_storageInfo->device();
    if (!m_hasStorageAccess || !m_storageInfo->isRemovable() || m_isRoot || !m_stateMonitor->isMounted(m_storageInfo->device().udi())) {
        auto access = device.as<Solid::StorageAccess>();
        if (access && access->canRepair() && m_stateMonitor->isChecked(m_storageInfo->device().udi())
            && m_stateMonitor->needRepair(m_storageInfo->device().udi()) && !m_stateMonitor->isMounted(m_storageInfo->device().udi())) {
            access->repair();
        } else {
            ActionInterface::triggered();
        }
    } else {
        if (device.is<Solid::OpticalDisc>()) {
            auto *drive = device.as<Solid::OpticalDrive>();
            if (!drive) {
                drive = device.parent().as<Solid::OpticalDrive>();
            }
            if (drive) {
                drive->eject();
            }
            return;
        }

        if (device.is<Solid::StorageAccess>()) {
            auto *access = device.as<Solid::StorageAccess>();
            if (access && access->isAccessible()) {
                access->teardown();
            }
        }
    }
}

void MountAndOpenAction::updateAction(const QString &udi)
{
    if (udi != m_storageInfo->device().udi()) {
        return;
    }
    qCDebug(APPLETS::DEVICENOTIFIER) << "Mount and open action: begin updating action";

    if (m_storageInfo->isRemovable()) {
        if (m_stateMonitor->isMounted(m_storageInfo->device().udi())) {
            m_icon = QStringLiteral("media-eject");
        } else {
            m_icon = (m_stateMonitor->isChecked(m_storageInfo->device().udi()) && m_stateMonitor->needRepair(m_storageInfo->device().udi()))
                ? QStringLiteral("tools-wizard")
                : QStringLiteral("document-open-folder");
        }
    } else {
        m_icon = QStringLiteral("document-open-folder");
    }

    // - It's possible for there to be no StorageAccess (e.g. MTP devices don't have one)
    // - It's possible for the root volume to be on a removable disk
    if (!m_hasStorageAccess || !m_storageInfo->isRemovable() || m_isRoot) {
        m_text = i18n("Open in File Manager");
    } else {
        if (!m_stateMonitor->isMounted(m_storageInfo->device().udi())) {
            m_text = (m_stateMonitor->isChecked(m_storageInfo->device().udi()) && m_stateMonitor->needRepair(m_storageInfo->device().udi()))
                ? i18n("Try to Fix")
                : i18n("Mount and Open");
        } else if (m_isOpticalDisk) {
            m_text = i18n("Eject");
        } else {
            m_text = i18n("Safely remove");
        }
    }
    qCDebug(APPLETS::DEVICENOTIFIER) << "Mount and open action: action updated! Icon: " << m_icon << ", Text: " << m_text;

    Q_EMIT iconChanged(m_icon);
    Q_EMIT textChanged(m_text);
}

void MountAndOpenAction::deviceStateChanged(const QString &udi)
{
    if (udi != m_storageInfo->device().udi() || m_stateMonitor->getState(m_storageInfo->device().udi()) != DevicesStateMonitor::CheckDone) {
        return;
    }

    qCDebug(APPLETS::DEVICENOTIFIER) << "Mount And Open action check done, need repair: " << m_stateMonitor->needRepair(m_storageInfo->device().udi());
    disconnect(m_stateMonitor.get(), &DevicesStateMonitor::stateChanged, this, &MountAndOpenAction::deviceStateChanged);
    if (!m_stateMonitor->needRepair(m_storageInfo->device().udi()) && !m_stateMonitor->isMounted(m_storageInfo->device().udi())) {
        ActionInterface::triggered();
    }
}

#include "moc_mountandopenaction.cpp"
