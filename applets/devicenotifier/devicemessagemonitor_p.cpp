/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "devicemessagemonitor_p.h"

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

DeviceMessageMonitor::DeviceMessageMonitor(QObject *parent)
    : QObject(parent)
    , m_deviceStateMonitor(DevicesStateMonitor::instance())
{
    connect(m_deviceStateMonitor.get(), &DevicesStateMonitor::stateChanged, this, &DeviceMessageMonitor::onStateChanged);
}

std::shared_ptr<DeviceMessageMonitor> DeviceMessageMonitor::instance()
{
    static std::weak_ptr<DeviceMessageMonitor> s_clip;
    if (s_clip.expired()) {
        std::shared_ptr<DeviceMessageMonitor> ptr{new DeviceMessageMonitor};
        s_clip = ptr;
        return ptr;
    }
    return s_clip.lock();
}

void DeviceMessageMonitor::addMonitoringDevice(const QString &udi)
{
    Solid::Device device(udi);

    if (!device.isValid()) {
        return;
    }

    // Check for messages that we potentially missed
    onStateChanged(udi);
}

void DeviceMessageMonitor::removeMonitoringDevice(const QString &udi)
{
    if (auto it = m_deviceMessages.constFind(udi); it != m_deviceMessages.cend()) {
        m_deviceMessages.erase(it);
    }
}

QString DeviceMessageMonitor::getMessage(const QString &udi)
{
    if (auto it = m_deviceMessages.constFind(udi); it != m_deviceMessages.cend()) {
        return it.value();
    }
    return {};
}

bool DeviceMessageMonitor::isSafelyRemovable(const QString &udi) const
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

void DeviceMessageMonitor::queryBlockingApps(const QString &devicePath)
{
    auto p = new QProcess;
    connect(p, &QProcess::errorOccurred, [p, this](QProcess::ProcessError) {
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

void DeviceMessageMonitor::clearPreviousMessage(const QString &udi)
{
    notify(std::nullopt, QString(), udi);
}

void DeviceMessageMonitor::onStateChanged(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Message Monitor: "
                                     << "State change signal arrived for device " << udi;

    if (m_deviceStateMonitor->isBusy(udi)) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Message Monitor: "
                                         << "The device in work. Reset the errors for " << udi;
        notify(QString(), QString(), udi);
        return;
    }

    auto operationResult = m_deviceStateMonitor->getOperationResult(udi);
    auto state = m_deviceStateMonitor->getState(udi);

    if (state == DevicesStateMonitor::Idle) {
        notify(QString(), QString(), udi);
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Message Monitor: "
                                         << "device " << udi << " is in the idle state. No message";
        return;
    }

    if (operationResult == Solid::ErrorType::NoError && state == DevicesStateMonitor::MountDone) {
        notify(QString(), QString(), udi);
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Message Monitor: "
                                         << "No message for device " << udi;
        return;
    }

    auto operationInfo = m_deviceStateMonitor->getOperationInfo(udi);

    // Bit awkward but oh well. The error message construction can defer an error to queryBlockingApps instead.
    // That makes it a tri-state return value between an error message, no error, and a deferred error.
    struct DeferredError {
    };

    const auto errorVariant = [&] -> std::variant<std::optional<QString>, DeferredError> {
        switch (operationResult) {
        case Solid::ErrorType::NoError:
            if (state == DevicesStateMonitor::CheckDone) {
                if (!operationInfo.toBool()) {
                    return i18n("This device has file system errors.");
                }
                return i18nc("@label device is a storage disk", "This device has no errors");
            }
            if (state == DevicesStateMonitor::RepairDone) {
                return i18n("Successfully repaired!");
            }
            if (state != DevicesStateMonitor::MountDone && isSafelyRemovable(udi)) {
                KNotification::event(QStringLiteral("safelyRemovable"),
                                     i18n("Device Status"),
                                     i18n("A device can now be safely removed"),
                                     u"device-notifier"_s,
                                     KNotification::CloseOnTimeout,
                                     u"devicenotifications"_s);
                return i18n("This device can now be safely removed.");
            }
            return std::nullopt;

        case Solid::ErrorType::UnauthorizedOperation:
            switch (state) {
            case DevicesStateMonitor::MountDone:
                return i18n("You are not authorized to mount this device.");
            case DevicesStateMonitor::UnmountDone: {
                Solid::Device device(udi);
                if (device.is<Solid::OpticalDisc>()) {
                    return i18n("You are not authorized to eject this disc.");
                }

                return i18nc("Remove is less technical for unmount", "You are not authorized to remove this device.");
            }
            case DevicesStateMonitor::RepairDone:
                return i18n("You are not authorized to repair this device.");
            default:
                return i18n("Unknown error type");
            }
        case Solid::ErrorType::DeviceBusy: {
            if (state == DevicesStateMonitor::MountDone) { // can this even happen?
                return i18n("Could not mount this device as it is busy.");
            }

            QString deviceUdi = udi;
            Solid::Device device(udi);
            if (state == DevicesStateMonitor::UnmountDone && device.is<Solid::OpticalDisc>()) {
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

            auto access = device.as<Solid::StorageAccess>();

            // Without that, our lambda function would capture an uninitialized object, resulting in UB
            // and random crashes
            auto c = new QMetaObject::Connection();
            *c = connect(this, &DeviceMessageMonitor::blockingAppsReady, [c, operationResult, operationInfo, deviceUdi, this](const QStringList &blockApps) {
                QString message;
                if (blockApps.isEmpty()) {
                    message = i18n("One or more files on this device are open within an application.");
                } else {
                    message = i18np("One or more files on this device are opened in application \"%2\".",
                                    "One or more files on this device are opened in following applications: %2.",
                                    blockApps.size(),
                                    blockApps.join(i18nc("separator in list of apps blocking device unmount", ", ")));
                }
                notify(message, operationInfo.toString(), deviceUdi);
                qCDebug(APPLETS::DEVICENOTIFIER) << "Device Message Monitor: " << "Message for device " << deviceUdi << " operation result: " << operationResult
                                                 << "message:" << message;
                disconnect(*c);
                delete c;
            });
            queryBlockingApps(access->filePath());
            return DeferredError{};
        }
        case Solid::ErrorType::UserCanceled: {
            // don't point out the obvious to the user, do nothing here
            break;
        }
        default: {
            switch (state) {
            case DevicesStateMonitor::MountDone:
                return i18n("Could not mount this device.");
            case DevicesStateMonitor::UnmountDone: {
                Solid::Device device(udi);
                if (device.is<Solid::OpticalDisc>()) {
                    return i18n("Could not eject this disc.");
                }
                return i18nc("Remove is less technical for unmount", "Could not remove this device.");
            }
            case DevicesStateMonitor::RepairDone:
                return i18n("Could not repair this device: %1").arg(operationInfo.toString());
            default:
                return i18n("Unknown error type");
            }
        }
        }

        return std::nullopt;
    }();

    if (std::holds_alternative<DeferredError>(errorVariant)) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Device Message Monitor: "
                                         << "Deferred error for device " << udi;
        return; // don't notify, we will do it later
    }

    const auto &message = std::get<std::optional<QString>>(errorVariant);

    qCDebug(APPLETS::DEVICENOTIFIER) << "Device Message Monitor: "
                                     << "message for device " << udi << " operation result: " << operationResult << " message:" << message;
    notify(message, operationInfo.toString(), udi);
}

void DeviceMessageMonitor::notify(const std::optional<QString> &message, const QString &description, const QString &udi)
{
    Q_UNUSED(description)

    if (message.has_value()) {
        m_deviceMessages[udi] = message.value();
    } else {
        if (auto it = m_deviceMessages.constFind(udi); it != m_deviceMessages.cend()) {
            m_deviceMessages.erase(it);
        }
    }

    Q_EMIT messageChanged(udi);
}

#include "moc_devicemessagemonitor_p.cpp"
