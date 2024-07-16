/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mountandopenaction.h"

#include <Solid/OpticalDisc>
#include <Solid/OpticalDrive>
#include <Solid/PortableMediaPlayer>

#include <KLocalizedString>

#include <devicenotifier_debug.h>

MountAndOpenAction::MountAndOpenAction(const QString &udi, QObject *parent)
    : ActionInterface(udi, parent)
    , m_stateMonitor(DevicesStateMonitor::instance())
{
    Solid::Device device(m_udi);

    m_hasStorageAccess = false;
    m_isOpticalDisk = false;
    m_isRoot = false;

    m_hasPortableMediaPlayer = false;

    if (device.is<Solid::StorageAccess>()) {
        Solid::StorageAccess *storageaccess = device.as<Solid::StorageAccess>();
        if (storageaccess) {
            m_hasStorageAccess = true;
            m_isRoot = storageaccess->filePath() == u"/";
        }

        if (device.is<Solid::OpticalDisc>()) {
            Solid::OpticalDisc *opticaldisc = device.as<Solid::OpticalDisc>();
            if (opticaldisc) {
                m_isOpticalDisk = true;
            }
        }
    }

    if (device.is<Solid::PortableMediaPlayer>()) {
        Solid::PortableMediaPlayer *mediaplayer = device.as<Solid::PortableMediaPlayer>();
        if (mediaplayer) {
            m_hasPortableMediaPlayer = true;
            m_supportedProtocols = mediaplayer->supportedProtocols();
        }
    }

    connect(m_stateMonitor.get(), &DevicesStateMonitor::stateChanged, this, &MountAndOpenAction::updateAction);

    updateAction(udi);
}

MountAndOpenAction::~MountAndOpenAction()
{
}

QString MountAndOpenAction::predicate() const
{
    QString newPredicate;

    if (!m_hasStorageAccess || !m_stateMonitor->isRemovable(m_udi) || !m_stateMonitor->isMounted(m_udi)) {
        newPredicate = QLatin1String("openWithFileManager.desktop");

        if (!m_hasStorageAccess && m_hasPortableMediaPlayer) {
            if (!m_supportedProtocols.isEmpty()) {
                return newPredicate;
            }

            for (auto protocol : m_supportedProtocols) {
                if (protocol == QLatin1String("mtp")) {
                    newPredicate = QLatin1String("solid_mtp.desktop"); // this lives in kio-extras!
                    break;
                }
                if (protocol == QLatin1String("afc")) {
                    newPredicate = QLatin1String("solid_afc.desktop"); // this lives in kio-extras!
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

    if (!m_hasStorageAccess || !m_stateMonitor->isRemovable(m_udi) || m_isRoot || !m_stateMonitor->isMounted(m_udi)) {
        ActionInterface::triggered();
    } else {
        Solid::Device device(m_udi);
        if (device.is<Solid::OpticalDisc>()) {
            Solid::OpticalDrive *drive = device.as<Solid::OpticalDrive>();
            if (!drive) {
                drive = device.parent().as<Solid::OpticalDrive>();
            }
            if (drive) {
                drive->eject();
            }
            return;
        }

        if (device.is<Solid::StorageAccess>()) {
            Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
            if (access && access->isAccessible()) {
                access->teardown();
            }
        }
    }
}

void MountAndOpenAction::updateAction(const QString &udi)
{
    if (udi != m_udi) {
        return;
    }
    qCDebug(APPLETS::DEVICENOTIFIER) << "Mount and open action: begin updating action";

    if (m_stateMonitor->isRemovable(m_udi)) {
        m_icon = m_stateMonitor->isMounted(m_udi) ? QStringLiteral("media-eject") : QStringLiteral("document-open-folder");
    } else {
        m_icon = QStringLiteral("document-open-folder");
    }

    // - It's possible for there to be no StorageAccess (e.g. MTP devices don't have one)
    // - It's possible for the root volume to be on a removable disk
    if (!m_hasStorageAccess || !m_stateMonitor->isRemovable(m_udi) || m_isRoot) {
        m_text = i18n("Open in File Manager");
    } else {
        if (!m_stateMonitor->isMounted(m_udi)) {
            m_text = i18n("Mount and Open");
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

#include "moc_mountandopenaction.cpp"
