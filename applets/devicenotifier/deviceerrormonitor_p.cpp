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

#include <KLocalizedString>
#include <KNotification>

#include <processcore/process.h>
#include <processcore/processes.h>

#include "devicestatemonitor_p.h"

using namespace Qt::StringLiterals;

DeviceErrorMonitor::DeviceErrorMonitor(QObject *parent)
    : QObject(parent)
{
    m_deviceStateMonitor = DevicesStateMonitor::instance();
    connect(m_deviceStateMonitor.get(), &DevicesStateMonitor::stateChanged, this, &DeviceErrorMonitor::onStateChanged);
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

    // Check for errors that we potentially missed
    onStateChanged(udi);
}

void DeviceErrorMonitor::removeMonitoringDevice(const QString &udi)
{
    if (auto it = m_deviceErrors.constFind(udi); it != m_deviceErrors.cend()) {
        m_deviceErrors.erase(it);
    }
}

QString DeviceErrorMonitor::getErrorMassage(const QString &udi)
{
    if (auto it = m_deviceErrors.constFind(udi); it != m_deviceErrors.cend()) {
        return it.value();
    }
    return {};
}

bool DeviceErrorMonitor::isSafelyRemovable(const QString &udi) const
{
    Solid::Device device(udi);
    if (device.is<Solid::StorageVolume>()) {
        auto drive = getAncestorAs<Solid::StorageDrive>(device);
        if (!drive /* Already removed from elsewhere */ || !drive->isValid()) {
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

void DeviceErrorMonitor::clearPreviousError(const QString &udi)
{
    notify(QString(), QString(), udi);
}

void DeviceErrorMonitor::onStateChanged(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Error Monitor: "
                                     << "State change signal arrived for device " << udi;

    if (m_deviceStateMonitor->isBusy(udi)) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Error Monitor: "
                                         << "The device in work. Reset the errors for " << udi;
        notify(QString(), QString(), udi);
        return;
    }

    auto result = m_deviceStateMonitor->getOperationResult(udi);
    auto error = m_deviceStateMonitor->getErrorType(udi);

    if (error == Solid::ErrorType::NoError && result == DevicesStateMonitor::MountDone) {
        notify(QString(), QString(), udi);
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Error Monitor: "
                                         << "No error for device " << udi;
        return;
    }

    auto errorData = m_deviceStateMonitor->getErrorData(udi);

    QString errorMsg;

    switch (error) {
    case Solid::ErrorType::NoError:
        if (result == DevicesStateMonitor::CheckDone) {
            if (!errorData.toBool()) {
                errorMsg = i18n("This device has file system errors.");
            }
        } else if (result == DevicesStateMonitor::RepairDone) {
            errorMsg = i18n("Successfully repaired!");
        } else if (result != DevicesStateMonitor::MountDone && isSafelyRemovable(udi)) {
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
        switch (result) {
        case DevicesStateMonitor::MountDone:
            errorMsg = i18n("You are not authorized to mount this device.");
            break;
        case DevicesStateMonitor::UnmountDone: {
            Solid::Device device(udi);
            if (device.is<Solid::OpticalDisc>()) {
                errorMsg = i18n("You are not authorized to eject this disc.");
                break;
            }

            errorMsg = i18nc("Remove is less technical for unmount", "You are not authorized to remove this device.");
            break;
        }
        case DevicesStateMonitor::RepairDone:
            errorMsg = i18n("You are not authorized to repair this device.");
            break;
        default:
            errorMsg = i18n("Unknown error type");
            break;
        }
        break;
    case Solid::ErrorType::DeviceBusy: {
        if (result == DevicesStateMonitor::MountDone) { // can this even happen?
            errorMsg = i18n("Could not mount this device as it is busy.");
        } else {
            QString deviceUdi = udi;
            Solid::Device device(udi);
            if (result == DevicesStateMonitor::UnmountDone && device.is<Solid::OpticalDisc>()) {
                const auto discs = Solid::Device::listFromType(Solid::DeviceInterface::OpticalDisc);
                for (const auto &disc : discs) {
                    if (disc.parentUdi() == udi) {
                        deviceUdi = disc.udi();
                        break;
                    }
                }

                if (deviceUdi.isNull()) {
                    Q_ASSERT_X(false, Q_FUNC_INFO, "This should not happen, bail out");
                }
            }

            Solid::StorageAccess *access = device.as<Solid::StorageAccess>();

            // Without that, our lambda function would capture an uninitialized object, resulting in UB
            // and random crashes
            QMetaObject::Connection *c = new QMetaObject::Connection();
            *c = connect(this, &DeviceErrorMonitor::blockingAppsReady, [c, error, errorData, deviceUdi, this](const QStringList &blockApps) {
                QString errorMessage;
                if (blockApps.isEmpty()) {
                    errorMessage = i18n("One or more files on this device are open within an application.");
                } else {
                    errorMessage = i18np("One or more files on this device are opened in application \"%2\".",
                                         "One or more files on this device are opened in following applications: %2.",
                                         blockApps.size(),
                                         blockApps.join(i18nc("separator in list of apps blocking device unmount", ", ")));
                }
                notify(errorMessage, errorData.toString(), deviceUdi);
                qCDebug(APPLETS::DEVICENOTIFIER) << "Device Error Monitor: " << "Error for device " << deviceUdi << " error: " << error
                                                 << " error message:" << errorMessage;
                disconnect(*c);
                delete c;
            });
            queryBlockingApps(access->filePath());
            return;
        }

        break;
    }
    case Solid::ErrorType::UserCanceled: {
        // don't point out the obvious to the user, do nothing here
        break;
    }
    default: {
        switch (result) {
        case DevicesStateMonitor::MountDone:
            errorMsg = i18n("Could not mount this device.");
            break;
        case DevicesStateMonitor::UnmountDone: {
            Solid::Device device(udi);
            if (device.is<Solid::OpticalDisc>()) {
                errorMsg = i18n("Could not eject this disc.");
                break;
            }
            errorMsg = i18nc("Remove is less technical for unmount", "Could not remove this device.");
            break;
        }
        case DevicesStateMonitor::RepairDone:
            errorMsg = i18n("Could not repair this device: %1").arg(errorData.toString());
            break;
        default:
            errorMsg = i18n("Unknown error type");
            break;
        }
        break;
    }
    }
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Error Monitor: "
                                     << "Error for device " << udi << " error: " << error << " error message:" << errorMsg;
    notify(errorMsg, errorData.toString(), udi);
}

void DeviceErrorMonitor::notify(const QString &errorMessage, const QString &description, const QString &udi)
{
    Q_UNUSED(description)

    if (!errorMessage.isEmpty()) {
        m_deviceErrors[udi] = errorMessage;
    } else {
        if (auto it = m_deviceErrors.constFind(udi); it != m_deviceErrors.cend()) {
            m_deviceErrors.erase(it);
        }
    }

    Q_EMIT errorDataChanged(udi);
}

#include "moc_deviceerrormonitor_p.cpp"
