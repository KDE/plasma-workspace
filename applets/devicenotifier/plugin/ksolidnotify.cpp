/*
    SPDX-FileCopyrightText: 2010 Jacopo De Simoi <wilderkde@gmail.com>
    SPDX-FileCopyrightText: 2014 Lukáš Tinkl <ltinkl@redhat.com>
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
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
#include <KNotification>
#include <processcore/process.h>
#include <processcore/processes.h>

#include <QProcess>
#include <QRegularExpression>
#include <QStringList>
#include <QStringView>

KSolidNotify::KSolidNotify(QObject *parent)
    : QObject(parent)
{
    Solid::Predicate p(Solid::DeviceInterface::StorageAccess);
    p |= Solid::Predicate(Solid::DeviceInterface::OpticalDrive);
    p |= Solid::Predicate(Solid::DeviceInterface::PortableMediaPlayer);
    for (const QList<Solid::Device> &devices = Solid::Device::listFromQuery(p); const Solid::Device &dev : devices) {
        connectSignals(*m_devices.insert(dev.udi(), dev));
    }

    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceAdded, this, &KSolidNotify::onDeviceAdded);
    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceRemoved, this, &KSolidNotify::onDeviceRemoved);
}

QString KSolidNotify::lastUdi() const
{
    return m_lastUdi;
}

Solid::ErrorType KSolidNotify::lastErrorType() const
{
    return m_lastErrorType;
}

QString KSolidNotify::lastMessage() const
{
    return m_lastMessage;
}

QString KSolidNotify::lastDescription() const
{
    return m_lastDescription;
}

QString KSolidNotify::lastIcon() const
{
    return m_lastIcon;
}

void KSolidNotify::clearMessage()
{
    if (m_lastUdi.isEmpty()) {
        return;
    }

    m_lastUdi.clear();
    m_lastErrorType = static_cast<Solid::ErrorType>(0);
    m_lastMessage.clear();
    m_lastDescription.clear();
    m_lastIcon.clear();
    Q_EMIT lastUdiChanged();
    Q_EMIT lastErrorTypeChanged();
    Q_EMIT lastMessageChanged();
    Q_EMIT lastDescriptionChanged();
    Q_EMIT lastIconChanged();
}

void KSolidNotify::onDeviceAdded(const QString &udi)
{
    // Clear any stale message from a previous instance
    clearMessage();

    connectSignals(*m_devices.emplace(udi, udi));
}

void KSolidNotify::onDeviceRemoved(const QString &udi)
{
    if (m_devices[udi].is<Solid::StorageVolume>()) {
        Solid::StorageAccess *access = m_devices[udi].as<Solid::StorageAccess>();
        if (access) {
            access->disconnect(this);
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

void KSolidNotify::connectSignals(Solid::Device &device)
{
    Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
    if (access) {
        connect(access, &Solid::StorageAccess::teardownDone, this, [this](Solid::ErrorType error, const QVariant &errorData, const QString &udi) {
            onSolidReply(SolidReplyType::Teardown, error, errorData, udi);
        });

        connect(access, &Solid::StorageAccess::setupDone, this, [this](Solid::ErrorType error, const QVariant &errorData, const QString &udi) {
            onSolidReply(SolidReplyType::Setup, error, errorData, udi);
        });
    }
    if (device.is<Solid::OpticalDisc>()) {
        Solid::OpticalDrive *drive = device.parent().as<Solid::OpticalDrive>();
        connect(drive, &Solid::OpticalDrive::ejectDone, this, [this](Solid::ErrorType error, const QVariant &errorData, const QString &udi) {
            onSolidReply(SolidReplyType::Eject, error, errorData, udi);
        });
    }
}

void KSolidNotify::queryBlockingApps(const QString &devicePath)
{
    QProcess *p = new QProcess;
    connect(p, static_cast<void (QProcess::*)(QProcess::ProcessError)>(&QProcess::errorOccurred), [=, this](QProcess::ProcessError) {
        Q_EMIT blockingAppsReady({});
        p->deleteLater();
    });
    connect(p, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), [p, this](int, QProcess::ExitStatus) {
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

void KSolidNotify::onSolidReply(SolidReplyType type, Solid::ErrorType error, const QVariant &errorData, const QString &udi)
{
    if ((error == Solid::ErrorType::NoError) && (type == SolidReplyType::Setup)) {
        if (m_lastUdi == udi) {
            clearMessage();
        }
        return;
    }

    QString errorMsg;
    QString icon;

    switch (error) {
    case Solid::ErrorType::NoError:
        if (type != SolidReplyType::Setup && isSafelyRemovable(udi)) {
            KNotification::event(QStringLiteral("safelyRemovable"), i18n("Device Status"), i18n("A device can now be safely removed"));
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
                for (const Solid::Device &device : std::as_const(m_devices)) {
                    if (device.parentUdi() == udi) {
                        discUdi = device.udi();
                    }
                }

                if (discUdi.isNull()) {
                    Q_ASSERT_X(false, Q_FUNC_INFO, "This should not happen, bail out");
                    return;
                }

                device = Solid::Device(discUdi);
            } else {
                device = Solid::Device(udi);
            }

            icon = device.icon();
            Solid::StorageAccess *access = device.as<Solid::StorageAccess>();

            // Without that, our lambda function would capture an uninitialized object, resulting in UB
            // and random crashes
            QMetaObject::Connection *c = new QMetaObject::Connection();
            *c = connect(this, &KSolidNotify::blockingAppsReady, [c, error, errorData, udi, icon, this](const QStringList &blockApps) {
                QString errorMessage;
                if (blockApps.isEmpty()) {
                    errorMessage = i18n("One or more files on this device are open within an application.");
                } else {
                    errorMessage = i18np("One or more files on this device are opened in application \"%2\".",
                                         "One or more files on this device are opened in following applications: %2.",
                                         blockApps.size(),
                                         blockApps.join(i18nc("separator in list of apps blocking device unmount", ", ")));
                }
                notify(error, errorMessage, errorData.toString(), udi, icon);
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

    notify(error, errorMsg, errorData.toString(), udi, icon);
}

void KSolidNotify::notify(Solid::ErrorType error, const QString &errorMessage, const QString &errorData, const QString &udi, const QString &icon)
{
    if (m_lastUdi != udi) {
        m_lastUdi = udi;
        Q_EMIT lastUdiChanged();
    }

    if (m_lastErrorType != error) {
        m_lastErrorType = error;
        Q_EMIT lastErrorTypeChanged();
    }

    if (errorMessage != m_lastMessage) {
        m_lastMessage = errorMessage;
        Q_EMIT lastMessageChanged();
    }

    if (m_lastDescription != errorData) {
        m_lastDescription = errorData;
        Q_EMIT lastDescriptionChanged();
    }

    if (m_lastIcon != icon) {
        m_lastIcon = icon;
        Q_EMIT lastIconChanged();
    }
}

#include "moc_ksolidnotify.cpp"
