/*
   Copyright (C) 2010 by Jacopo De Simoi <wilderkde@gmail.com>
   Copyright (C) 2014 by Lukáš Tinkl <ltinkl@redhat.com>
   Copyright (C) 2016 by Kai Uwe Broulik <kde@privat.broulik.de>

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

#include <Solid/DeviceInterface>
#include <Solid/DeviceNotifier>
#include <Solid/OpticalDisc>
#include <Solid/OpticalDrive>
#include <Solid/PortableMediaPlayer>
#include <Solid/Predicate>
#include <Solid/StorageAccess>
#include <Solid/StorageDrive>
#include <Solid/StorageVolume>

#include <KLocalizedString>
#include <processcore/process.h>
#include <processcore/processes.h>

#include <QProcess>
#include <QRegularExpression>
#include <QStringList>
#include <QStringRef>
#include <QVector>

KSolidNotify::KSolidNotify(QObject *parent)
    : QObject(parent)
{
    Solid::Predicate p(Solid::DeviceInterface::StorageAccess);
    p |= Solid::Predicate(Solid::DeviceInterface::OpticalDrive);
    p |= Solid::Predicate(Solid::DeviceInterface::PortableMediaPlayer);
    const QList<Solid::Device> &devices = Solid::Device::listFromQuery(p);
    for (const Solid::Device &dev : devices) {
        m_devices.insert(dev.udi(), dev);
        connectSignals(&m_devices[dev.udi()]);
    }

    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceAdded, this, &KSolidNotify::onDeviceAdded);
    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceRemoved, this, &KSolidNotify::onDeviceRemoved);
}

void KSolidNotify::onDeviceAdded(const QString &udi)
{
    // Clear any stale message from a previous instance
    emit clearNotification(udi);
    Solid::Device device(udi);
    m_devices.insert(udi, device);
    connectSignals(&m_devices[udi]);
}

void KSolidNotify::onDeviceRemoved(const QString &udi)
{
    if (m_devices[udi].is<Solid::StorageVolume>()) {
        Solid::StorageAccess *access = m_devices[udi].as<Solid::StorageAccess>();
        if (access) {
            disconnect(access, nullptr, this, nullptr);
        }
    }
    m_devices.remove(udi);
}

bool KSolidNotify::isSafelyRemovable(const QString &udi) const
{
    Solid::Device parent = m_devices[udi].parent();
    if (parent.is<Solid::StorageDrive>()) {
        Solid::StorageDrive *drive = parent.as<Solid::StorageDrive>();
        return (!drive->isInUse() && (drive->isHotpluggable() || drive->isRemovable()));
    }

    const Solid::StorageAccess *access = m_devices[udi].as<Solid::StorageAccess>();
    if (access) {
        return !m_devices[udi].as<Solid::StorageAccess>()->isAccessible();
    } else {
        // If this check fails, the device has been already physically
        // ejected, so no need to say that it is safe to remove it
        return false;
    }
}

void KSolidNotify::connectSignals(Solid::Device *device)
{
    Solid::StorageAccess *access = device->as<Solid::StorageAccess>();
    if (access) {
        connect(access, &Solid::StorageAccess::teardownDone, this, [=](Solid::ErrorType error, const QVariant &errorData, const QString &udi) {
            onSolidReply(SolidReplyType::Teardown, error, errorData, udi);
        });

        connect(access, &Solid::StorageAccess::setupDone, this, [=](Solid::ErrorType error, const QVariant &errorData, const QString &udi) {
            onSolidReply(SolidReplyType::Setup, error, errorData, udi);
        });
    }
    if (device->is<Solid::OpticalDisc>()) {
        Solid::OpticalDrive *drive = device->parent().as<Solid::OpticalDrive>();
        connect(drive, &Solid::OpticalDrive::ejectDone, this, [=](Solid::ErrorType error, const QVariant &errorData, const QString &udi) {
            onSolidReply(SolidReplyType::Eject, error, errorData, udi);
        });
    }
}

void KSolidNotify::queryBlockingApps(const QString &devicePath)
{
    QProcess *p = new QProcess;
    connect(p, static_cast<void (QProcess::*)(QProcess::ProcessError)>(&QProcess::errorOccurred), [=](QProcess::ProcessError) {
        emit blockingAppsReady({});
        p->deleteLater();
    });
    connect(p, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), [=](int, QProcess::ExitStatus) {
        QStringList blockApps;
        QString out(p->readAll());
        const QVector<QStringRef> pidList = out.splitRef(QRegularExpression(QStringLiteral("\\s+")), QString::SkipEmptyParts);
        KSysGuard::Processes procs;
        for (const QStringRef &pidStr : pidList) {
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

void KSolidNotify::onSolidReply(SolidReplyType type, Solid::ErrorType error, const QVariant &errorData, const QString &udi)
{
    if ((error == Solid::ErrorType::NoError) && (type == SolidReplyType::Setup)) {
        emit clearNotification(udi);
        return;
    }

    QString errorMsg;

    switch (error) {
    case Solid::ErrorType::NoError:
        if (type != SolidReplyType::Setup && isSafelyRemovable(udi)) {
            errorMsg = i18n("This device can now be safely removed.");
        }
        break;

    case Solid::ErrorType::UnauthorizedOperation:
        switch (type) {
        case SolidReplyType::Setup:
            errorMsg = i18n("You are not authorized to mount this device.");
            break;
        case SolidReplyType::Teardown:
            errorMsg = i18nc("Remove is less technical for unmount", "You are not authorized to remove this device.");
            break;
        case SolidReplyType::Eject:
            errorMsg = i18n("You are not authorized to eject this disc.");
            break;
        }

        break;
    case Solid::ErrorType::DeviceBusy: {
        if (type == SolidReplyType::Setup) { // can this even happen?
            errorMsg = i18n("Could not mount this device as it is busy.");
        } else {
            Solid::Device device;

            if (type == SolidReplyType::Eject) {
                QString discUdi;
                for (Solid::Device device : qAsConst(m_devices)) {
                    if (device.parentUdi() == udi) {
                        discUdi = device.udi();
                    }
                }

                if (discUdi.isNull()) {
                    // This should not happen, bail out
                    return;
                }

                device = Solid::Device(discUdi);
            } else {
                device = Solid::Device(udi);
            }

            Solid::StorageAccess *access = device.as<Solid::StorageAccess>();

            // Without that, our lambda function would capture an uninitialized object, resulting in UB
            // and random crashes
            QMetaObject::Connection *c = new QMetaObject::Connection();
            *c = connect(this, &KSolidNotify::blockingAppsReady, [=](const QStringList &blockApps) {
                QString errorMessage;
                if (blockApps.isEmpty()) {
                    errorMessage = i18n("One or more files on this device are open within an application.");
                } else {
                    errorMessage = i18np("One or more files on this device are opened in application \"%2\".",
                                         "One or more files on this device are opened in following applications: %2.",
                                         blockApps.count(),
                                         blockApps.join(i18nc("separator in list of apps blocking device unmount", ", ")));
                }
                emit notify(error, errorMessage, errorData.toString(), udi);
                disconnect(*c);
                delete c;
            });
            queryBlockingApps(access->filePath());
        }

        break;
    }
    case Solid::ErrorType::UserCanceled:
        // don't point out the obvious to the user, do nothing here
        break;
    default:
        switch (type) {
        case SolidReplyType::Setup:
            errorMsg = i18n("Could not mount this device.");
            break;
        case SolidReplyType::Teardown:
            errorMsg = i18nc("Remove is less technical for unmount", "Could not remove this device.");
            break;
        case SolidReplyType::Eject:
            errorMsg = i18n("Could not eject this disc.");
            break;
        }

        break;
    }

    if (!errorMsg.isEmpty()) {
        emit notify(error, errorMsg, errorData.toString(), udi);
    }
}
