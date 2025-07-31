/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "messageinfo.h"

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

using namespace Qt::StringLiterals;

MessageInfo::MessageInfo(const std::shared_ptr<StorageInfo> &storageInfo, const std::shared_ptr<StateInfo> &stateInfo, QObject *parent)
    : QObject(parent)
    , m_storageInfo(storageInfo)
    , m_stateInfo(stateInfo)
{
    connect(m_stateInfo.get(), &StateInfo::stateChanged, this, &MessageInfo::onStateChanged);
}

QString MessageInfo::getMessage() const
{
    return m_message;
}

void MessageInfo::queryBlockingApps(const QString &devicePath)
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

void MessageInfo::clearPreviousMessage()
{
    notify(std::nullopt);
}

void MessageInfo::onStateChanged()
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Message Info " << m_storageInfo->device().udi() << " : State change signal arrived";

    if (m_stateInfo->isBusy()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Message Info " << m_storageInfo->device().udi() << " : The device in work. Reset the errors";
        notify(QString());
        return;
    }

    auto operationResult = m_stateInfo->getOperationResult();
    auto state = m_stateInfo->getState();

    if (operationResult == Solid::ErrorType::NoError && state == StateInfo::MountDone) {
        notify(QString());
        qCDebug(APPLETS::DEVICENOTIFIER) << "Message Info " << m_storageInfo->device().udi() << " : No message for device";
        return;
    }

    auto operationInfo = m_stateInfo->getOperationInfo();

    // Bit awkward but oh well. The error message construction can defer an error to queryBlockingApps instead.
    // That makes it a tri-state return value between an error message, no error, and a deferred error.
    struct DeferredError {
    };

    const auto errorVariant = [&] -> std::variant<std::optional<QString>, DeferredError> {
        switch (operationResult) {
        case Solid::ErrorType::NoError:
            if (state == StateInfo::CheckDone) {
                if (!operationInfo.toBool()) {
                    return i18n("This device has file system errors.");
                }
                return i18nc("@label device is a storage disk", "This device has no errors");
            }
            if (state == StateInfo::RepairDone) {
                return i18n("Successfully repaired!");
            }
            if (state == StateInfo::UnmountDone && m_stateInfo->isSafelyRemovable()) {
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
            case StateInfo::MountDone:
                return i18n("You are not authorized to mount this device.");
            case StateInfo::UnmountDone: {
                const Solid::Device &device = m_storageInfo->device();
                if (device.is<Solid::OpticalDisc>()) {
                    return i18n("You are not authorized to eject this disc.");
                }

                return i18nc("Remove is less technical for unmount", "You are not authorized to remove this device.");
            }
            case StateInfo::RepairDone:
                return i18n("You are not authorized to repair this device.");
            default:
                return i18n("Unknown error type");
            }
        case Solid::ErrorType::DeviceBusy: {
            if (state == StateInfo::MountDone) { // can this even happen?
                return i18n("Could not mount this device as it is busy.");
            }

            QString deviceUdi = m_storageInfo->device().udi();
            Solid::Device device = m_storageInfo->device();

            if (state == StateInfo::UnmountDone && device.is<Solid::OpticalDisc>()) {
                const auto discs = Solid::Device::listFromType(Solid::DeviceInterface::OpticalDisc);
                for (const auto &disc : discs) {
                    if (disc.parentUdi() == m_storageInfo->device().udi()) {
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
            *c = connect(this, &MessageInfo::blockingAppsReady, [c, operationResult, operationInfo, deviceUdi, this](const QStringList &blockApps) {
                QString message;
                if (blockApps.isEmpty()) {
                    message = i18n("One or more files on this device are open within an application.");
                } else {
                    message = i18np("One or more files on this device are opened in application \"%2\".",
                                    "One or more files on this device are opened in following applications: %2.",
                                    blockApps.size(),
                                    blockApps.join(i18nc("separator in list of apps blocking device unmount", ", ")));
                }
                notify(message);
                qCDebug(APPLETS::DEVICENOTIFIER) << "Message Info " << m_storageInfo->device().udi() << " : operation result: " << operationResult
                                                 << "message:" << message;
                disconnect(*c);
                delete c;
            });
            qCDebug(APPLETS::DEVICENOTIFIER) << "Message Info " << m_storageInfo->device().udi() << " : Querying for blocking apps";
            queryBlockingApps(access->filePath());
            return DeferredError{};
        }
        case Solid::ErrorType::UserCanceled: {
            // don't point out the obvious to the user, do nothing here
            break;
        }
        default: {
            switch (state) {
            case StateInfo::MountDone:
                return i18n("Could not mount this device.");
            case StateInfo::UnmountDone: {
                const Solid::Device &device = m_storageInfo->device();
                if (device.is<Solid::OpticalDisc>()) {
                    return i18n("Could not eject this disc.");
                }
                return i18nc("Remove is less technical for unmount", "Could not remove this device.");
            }
            case StateInfo::RepairDone:
                return i18n("Could not repair this device: %1").arg(operationInfo.toString());
            default:
                return i18n("Unknown error type");
            }
        }
        }

        return std::nullopt;
    }();

    if (std::holds_alternative<DeferredError>(errorVariant)) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Message Info " << m_storageInfo->device().udi() << " : Deferred error for device";
        return; // don't notify, we will do it later
    }

    const auto &message = std::get<std::optional<QString>>(errorVariant);

    qCDebug(APPLETS::DEVICENOTIFIER) << "Message Info " << m_storageInfo->device().udi() << " : operation result: " << operationResult
                                     << " message:" << message;
    notify(message);
}

void MessageInfo::notify(const std::optional<QString> &message)
{
    if (message.has_value()) {
        m_message = message.value();
    } else {
        m_message.clear();
    }

    Q_EMIT messageChanged(m_storageInfo->device().udi());
}

#include "moc_messageinfo.cpp"
