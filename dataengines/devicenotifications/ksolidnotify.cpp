/*
   Copyright (C) 2010 by Jacopo De Simoi <wilderkde@gmail.com>
   Copyright (C) 2014 by Lukáš Tinkl <ltinkl@redhat.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 */

#include "ksolidnotify.h"

#include <Solid/DeviceNotifier>
#include <Solid/DeviceInterface>
#include <Solid/StorageDrive>
#include <Solid/StorageVolume>
#include <Solid/StorageAccess>
#include <Solid/OpticalDrive>
#include <Solid/OpticalDisc>
#include <Solid/PortableMediaPlayer>
#include <Solid/Predicate>

#include <KLocalizedString>
#include <processcore/process.h>
#include <processcore/processes.h>

#include <QStringList>
#include <QProcess>

KSolidNotify::KSolidNotify(QObject* parent):
    QObject(parent)
{
    Solid::Predicate p(Solid::DeviceInterface::StorageAccess);
    p |= Solid::Predicate(Solid::DeviceInterface::OpticalDrive);
    p |= Solid::Predicate(Solid::DeviceInterface::PortableMediaPlayer);
    QList<Solid::Device> devices = Solid::Device::listFromQuery(p);
    foreach (const Solid::Device &dev, devices)
    {
        m_devices.insert(dev.udi(), dev);
        connectSignals(&m_devices[dev.udi()]);
    }

    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(const QString &)),
            this, SLOT(onDeviceAdded(const QString &)));
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(const QString &)),
            this, SLOT(onDeviceRemoved(const QString &)));
}

void KSolidNotify::onDeviceAdded(const QString &udi)
{
    Solid::Device device(udi);
    m_devices.insert(udi, device);
    connectSignals(&m_devices[udi]);
}

void KSolidNotify::onDeviceRemoved(const QString &udi)
{
    if (m_devices[udi].is<Solid::StorageVolume>())
    {
        Solid::StorageAccess *access = m_devices[udi].as<Solid::StorageAccess>();
        if (access)
            disconnect(access, 0, this, 0);
    }
    m_devices.remove(udi);
}

bool KSolidNotify::isSafelyRemovable(const QString &udi)
{
    Solid::Device parent = m_devices[udi].parent();
    if (parent.is<Solid::StorageDrive>())
    {
        Solid::StorageDrive *drive = parent.as<Solid::StorageDrive>();
        return (!drive->isInUse() && (drive->isHotpluggable() || drive->isRemovable()));
    }

    Solid::StorageAccess* access = m_devices[udi].as<Solid::StorageAccess>();
    if (access) {
        return !m_devices[udi].as<Solid::StorageAccess>()->isAccessible();
    } else {
        // If this check fails, the device has been already physically
        // ejected, so no need to say that it is safe to remove it
        return false;
    }
}

void KSolidNotify::connectSignals(Solid::Device* device)
{
    Solid::StorageAccess *access = device->as<Solid::StorageAccess>();
    if (access) {
        connect(access, SIGNAL(teardownDone(Solid::ErrorType, QVariant, const QString &)),
                this, SLOT(storageTeardownDone(Solid::ErrorType, QVariant , const QString &)));
        connect(access, SIGNAL(setupDone(Solid::ErrorType, QVariant, const QString &)),
                this, SLOT(storageSetupDone(Solid::ErrorType, QVariant , const QString &)));
    }
    if (device->is<Solid::OpticalDisc>()) {
        Solid::OpticalDrive *drive = device->parent().as<Solid::OpticalDrive>();
        connect(drive, SIGNAL(ejectDone(Solid::ErrorType, QVariant, const QString &)),
                this, SLOT(storageEjectDone(Solid::ErrorType, QVariant , const QString &)));
    }
}

void KSolidNotify::storageSetupDone(Solid::ErrorType error, QVariant errorData, const QString &udi)
{
    if (error) {
        Solid::Device device(udi);
        const QString errorMessage = i18n("Could not mount the following device: %1", device.description());
        emit notify(error, errorMessage, errorData.toString(), udi);
    }
}

void KSolidNotify::queryBlockingApps(const QString &devicePath)
{
    QProcess *p = new QProcess;
    connect(p, static_cast<void (QProcess::*)(QProcess::ProcessError)>(&QProcess::error), [=](QProcess::ProcessError) {
                emit blockingAppsReady({});
                p->deleteLater();
            });
    connect(p, static_cast<void (QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished), [=](int,QProcess::ExitStatus) {
                QStringList blockApps;
                QString out(p->readAll());
                const QStringList &pidList = out.split(QRegExp("\\s+"), QString::SkipEmptyParts);
                KSysGuard::Processes procs;
                Q_FOREACH (const QString &pidStr, pidList) {
                    int pid = pidStr.toInt();
                    if (!pid) {
                        continue;
                    }
                    procs.updateOrAddProcess(pid);
                    KSysGuard::Process *proc = procs.getProcess(pid);
                    if (!blockApps.contains(proc->name())) {
                        blockApps << proc->name();
                    }
                }
                blockApps.removeDuplicates();
                emit blockingAppsReady(blockApps);
                p->deleteLater();
            });
    p->start(QStringLiteral("lsof"), {QStringLiteral("-t"), devicePath});
//    p.start(QStringLiteral("fuser"), {QStringLiteral("-m"), devicePath});
}

void KSolidNotify::storageTeardownDone(Solid::ErrorType error, QVariant errorData, const QString &udi)
{
    if (error) {
        Solid::Device device(udi);
        Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
        // Without that, our lambda function would capture an uninitialized object, resulting in UB
        // and random crashes
        QMetaObject::Connection *c = new QMetaObject::Connection();
        *c = connect(this, &KSolidNotify::blockingAppsReady,
                [=] (const QStringList &blockApps) {
                    QString errorMessage;
                    if (blockApps.isEmpty()) {
                        errorMessage = i18n("Could not safely remove the following device: %1\nOne or more files on this device are open within an application", device.description());
                    } else {
                        errorMessage = i18np("Could not safely remove the following device: %1\nOne or more files on this device are opened in application \"%3\"\nPlease close it and try again",
                                "Could not safely remove the following device: %1\nOne or more files on this device are opened in following applications: %3\nPlease close it and try again",
                                device.description(), blockApps.size(), blockApps.join(i18nc("separator in list of apps blocking device unmount", ", ")));
                    }
                    emit notify(error, errorMessage, errorData.toString(), udi);
                    disconnect(*c);
                    delete c;
                });
        queryBlockingApps(access->filePath());
    } else if (isSafelyRemovable(udi)) {
        Solid::Device device(udi);
        const QString errorMessage = i18nc("The term \"remove\" here means \"physically disconnect the device from the computer\", whereas \"safely\" means \"without risk of data loss\"", "The following device can now be safely removed: %1", device.description());
        emit notify(error, errorMessage, errorData.toString(), udi);
    }
}

void KSolidNotify::storageEjectDone(Solid::ErrorType error, QVariant errorData, const QString &udi)
{
    if (error)
    {
        QString discUdi;
        foreach (Solid::Device device, m_devices) {
            if (device.parentUdi() == udi) {
                discUdi = device.udi();
            }
        }

        if (discUdi.isNull()) {
            //This should not happen, bail out
            return;
        }

        Solid::Device discDevice(discUdi);
        Solid::StorageAccess *access = discDevice.as<Solid::StorageAccess>();
        // Without that, our lambda function would capture an uninitialized object, resulting in UB
        // and random crashes
        QMetaObject::Connection *c = new QMetaObject::Connection();
        *c = connect(this, &KSolidNotify::blockingAppsReady,
                [=] (const QStringList &blockApps) {
                    QString errorMessage;
                    if (blockApps.isEmpty()) {
                        errorMessage = i18n("Could not eject the following device: %1\nOne or more files on this device are open within an application", discDevice.description());
                    } else {
                        errorMessage = i18np("Could not eject the following device: %1\nOne or more files on this device are opened in application \"%3\"\nPlease close it and try again",
                                "Could not eject the following device: %1\nOne or more files on this device are opened in following applications: %3\nPlease close it and try again",
                                discDevice.description(), blockApps.size(), blockApps.join(i18nc("separator in list of apps blocking device unmount", ", ")));
                    }
                    emit notify(error, errorMessage, errorData.toString(), udi);
                    disconnect(*c);
                    delete c;
                });
        queryBlockingApps(access->filePath());
    } else if (isSafelyRemovable(udi)) {
        Solid::Device device(udi);
        const QString errorMessage = i18n("The following device can now be safely removed: %1", device.description());
        emit notify(error, errorMessage, errorData.toString(), udi);
    }
}
