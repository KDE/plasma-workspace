/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "deviceerrormonitor_p.h"

#include <QProcess>
#include <QRegularExpression>
#include <QStringView>

#include "devicenotifier_debug.h"

#include <Solid/OpticalDisc>
#include <Solid/OpticalDrive>
#include <Solid/StorageDrive>
#include <Solid/StorageVolume>

#include <KNotification>
#include <KLocalizedString>

#include <processcore/process.h>
#include <processcore/processes.h>

#include "devicestatemonitor_p.h"

using namespace Qt::StringLiterals;

DeviceErrorMonitor::DeviceErrorMonitor(QObject *parent)
    : QObject(parent)
{
}

DeviceErrorMonitor::~DeviceErrorMonitor()
{
}

std::shared_ptr<DeviceErrorMonitor> DeviceErrorMonitor::instance()
{
    static std::weak_ptr<DeviceErrorMonitor> s_clip;
    if (s_clip.expired()) {
        std::shared_ptr<DeviceErrorMonitor> ptr{new DeviceErrorMonitor};
        s_clip = ptr;
        return ptr;
    }
    return s_clip.lock();
}

void DeviceErrorMonitor::addMonitoringDevice(const QString &udi)
{
    Solid::Device device(udi);

    if (!device.isValid()) {
        return;
    }

    Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
    if (access) {
        connect(access, &Solid::StorageAccess::teardownDone, this, [this](Solid::ErrorType error, const QVariant &errorData, const QString &udi) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Error Monitor: "
                                             << "Teardown signal arrived for device " << udi;
            onSolidReply(SolidReplyType::Teardown, error, errorData, udi);
        });

        connect(access, &Solid::StorageAccess::setupDone, this, [this](Solid::ErrorType error, const QVariant &errorData, const QString &udi) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Device Error Monitor: "
                                             << "Setup signal arrived for device " << udi;
            onSolidReply(SolidReplyType::Setup, error, errorData, udi);
        });
    }
    if (device.is<Solid::OpticalDisc>()) {
        Solid::OpticalDrive *drive = device.parent().as<Solid::OpticalDrive>();
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Error Monitor: "
                                         << "Eject signal arrived for device " << udi;
        connect(drive, &Solid::OpticalDrive::ejectDone, this, [this](Solid::ErrorType error, const QVariant &errorData, const QString &udi) {
            onSolidReply(SolidReplyType::Eject, error, errorData, udi);
        });
    }
}

void DeviceErrorMonitor::removeMonitoringDevice(const QString &udi)
{
    Solid::Device device(udi);
    if (device.is<Solid::StorageVolume>()) {
        Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
        if (access) {
            access->disconnect(this);
        }
    }
    if (auto it = m_deviceErrors.constFind(udi); it != m_deviceErrors.cend()) {
        m_deviceErrors.erase(it);
    }
}

Solid::ErrorType DeviceErrorMonitor::getError(const QString &udi)
{
    if (auto it = m_deviceErrors.constFind(udi); it != m_deviceErrors.cend()) {
        return it->first;
    }
    return Solid::ErrorType::NoError;
}

QString DeviceErrorMonitor::getErrorMassage(const QString &udi)
{
    if (auto it = m_deviceErrors.constFind(udi); it != m_deviceErrors.cend()) {
        return it->second;
    }
    return {};
}

bool DeviceErrorMonitor::isSafelyRemovable(const QString &udi) const
{
    Solid::Device device(udi);
    if (device.is<Solid::StorageVolume>()) {
        auto drive = getAncestorAs<Solid::StorageDrive>(device);
        if (!drive->isValid()) {
            return true;
        }
        return !drive->isInUse() && (drive->isHotpluggable() || drive->isRemovable());
    }

    auto access = device.as<Solid::StorageAccess>();
    if (access) {
        return !access->isAccessible();
    }
    // If this check fails, the device has been already physically
    // ejected, so no need to say that it is safe to remove it
    return false;
}

void DeviceErrorMonitor::queryBlockingApps(const QString &devicePath)
{
    QProcess *p = new QProcess;
    connect(p, &QProcess::errorOccurred, [=, this](QProcess::ProcessError) {
        Q_EMIT blockingAppsReady({});
        p->deleteLater();
    });
    connect(p, &QProcess::finished, [p, this](int, QProcess::ExitStatus) {
        QStringList blockApps;
        const QString out = QString::fromLatin1(p->readAll());
        const auto pidList = QStringView(out).split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        KSysGuard::Processes procs;
        for (const QStringView pidStr : pidList) {
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
        Q_EMIT blockingAppsReady(blockApps);
        p->deleteLater();
    });
    p->start(QStringLiteral("lsof"), {QStringLiteral("-t"), devicePath});
    //    p.start(QStringLiteral("fuser"), {QStringLiteral("-m"), devicePath});
}

void DeviceErrorMonitor::onSolidReply(SolidReplyType type, Solid::ErrorType error, const QVariant &errorData, const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Error Monitor: "
                                     << "Reply arrived for device " << udi << " arrived";

    if (error == Solid::ErrorType::NoError && type == SolidReplyType::Setup) {
        notify(Solid::ErrorType::NoError, QString(), QString(), udi);
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Error Monitor: "
                                         << "No error for device " << udi;
        return;
    }

    QString errorMsg;

    switch (error) {
    case Solid::ErrorType::NoError:
        if (type != SolidReplyType::Setup && isSafelyRemovable(udi)) {
            KNotification::event(QStringLiteral("safelyRemovable"),
                                 i18n("Device Status"),
                                 i18n("A device can now be safely removed"),
                                 u"device-notifier"_s,
                                 KNotification::CloseOnTimeout,
                                 u"devicenotifications"_s);
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
                QString discUdi = device.parentUdi();

                if (discUdi.isNull()) {
                    Q_ASSERT_X(false, Q_FUNC_INFO, "This should not happen, bail out");
                }

                device = Solid::Device(discUdi);
            } else {
                device = Solid::Device(udi);
            }

            Solid::StorageAccess *access = device.as<Solid::StorageAccess>();

            // Without that, our lambda function would capture an uninitialized object, resulting in UB
            // and random crashes
            QMetaObject::Connection *c = new QMetaObject::Connection();
            *c = connect(this, &DeviceErrorMonitor::blockingAppsReady, [c, error, errorData, udi, this](const QStringList &blockApps) {
                QString errorMessage;
                if (blockApps.isEmpty()) {
                    errorMessage = i18n("One or more files on this device are open within an application.");
                } else {
                    errorMessage = i18np("One or more files on this device are opened in application \"%2\".",
                                         "One or more files on this device are opened in following applications: %2.",
                                         blockApps.size(),
                                         blockApps.join(i18nc("separator in list of apps blocking device unmount", ", ")));
                }
                notify(error, errorMessage, errorData.toString(), udi);
                qCDebug(APPLETS::DEVICENOTIFIER) << "Device Error Monitor: "
                                                 << "Error for device " << udi << " error: " << error << " error message:" << errorMessage;
                disconnect(*c);
                delete c;
            });
            queryBlockingApps(access->filePath());
            return;
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
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Error Monitor: "
                                     << "Error for device " << udi << " error: " << error << " error message:" << errorMsg;
    notify(error, errorMsg, errorData.toString(), udi);
}

void DeviceErrorMonitor::notify(Solid::ErrorType error, const QString &errorMessage, const QString &description, const QString &udi)
{
    Q_UNUSED(description)

    if (!errorMessage.isEmpty()) {
        m_deviceErrors[udi].first = error;
        m_deviceErrors[udi].second = errorMessage;
    } else {
        if (auto it = m_deviceErrors.constFind(udi); it != m_deviceErrors.cend()) {
            m_deviceErrors.erase(it);
        }
    }

    Q_EMIT errorDataChanged(udi);
}
