/*
    SPDX-FileCopyrightText: 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
    SPDX-FileCopyrightText: 2005-2007 Kevin Ottens <ervin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "deviceserviceaction.h"

#include <QDebug>

#include <KApplicationTrader>
#include <KConfigGroup>
#include <KDesktopFile>
#include <KIO/CommandLauncherJob>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KService>
#include <kmacroexpander.h>
#include <solid/block.h>
#include <solid/device.h>
#include <solid/storageaccess.h>

class MacroExpander : public KMacroExpanderBase
{
public:
    MacroExpander(const Solid::Device &device)
        : KMacroExpanderBase('%')
        , m_device(device)
    {
    }

protected:
    int expandEscapedMacro(const QString &str, int pos, QStringList &ret) override;

private:
    Solid::Device m_device;
};

class DelayedExecutor : public QObject
{
    Q_OBJECT
public:
    DelayedExecutor(const KServiceAction &service, Solid::Device &device);

private Q_SLOTS:
    void _k_storageSetupDone(Solid::ErrorType error, QVariant errorData, const QString &udi);

private:
    void delayedExecute(const QString &udi);

    KServiceAction m_service;
};

void DeviceServiceAction::execute(Solid::Device &device)
{
    new DelayedExecutor(m_service, device);
}

void DelayedExecutor::_k_storageSetupDone(Solid::ErrorType error, QVariant errorData, const QString &udi)
{
    Q_UNUSED(errorData);

    if (!error) {
        delayedExecute(udi);
    }
}

void DeviceServiceAction::setService(const KServiceAction &service)
{
    m_service = service;
}

KServiceAction DeviceServiceAction::service() const
{
    return m_service;
}

int MacroExpander::expandEscapedMacro(const QString &str, int pos, QStringList &ret)
{
    ushort option = str[pos + 1].unicode();

    switch (option) {
    case 'f': // Filepath
    case 'F': // case insensitive
        if (m_device.is<Solid::StorageAccess>()) {
            ret << m_device.as<Solid::StorageAccess>()->filePath();
        } else {
            qWarning() << "DeviceServiceAction::execute: " << m_device.udi() << " is not a StorageAccess device";
        }
        break;
    case 'd': // Device node
    case 'D': // case insensitive
        if (m_device.is<Solid::Block>()) {
            ret << m_device.as<Solid::Block>()->device();
        } else {
            qWarning() << "DeviceServiceAction::execute: " << m_device.udi() << " is not a Block device";
        }
        break;
    case 'i': // UDI
    case 'I': // case insensitive
        ret << m_device.udi();
        break;
    case '%':
        ret = QStringList(QLatin1String("%"));
        break;
    default:
        return -2; // subst with same and skip
    }
    return 2;
}

DelayedExecutor::DelayedExecutor(const KServiceAction &service, Solid::Device &device)
    : m_service(service)
{
    if (device.is<Solid::StorageAccess>() && !device.as<Solid::StorageAccess>()->isAccessible()) {
        Solid::StorageAccess *access = device.as<Solid::StorageAccess>();

        connect(access, &Solid::StorageAccess::setupDone, this, &DelayedExecutor::_k_storageSetupDone);

        access->setup();
    } else {
        delayedExecute(device.udi());
    }
}

void DelayedExecutor::delayedExecute(const QString &udi)
{
    Solid::Device device(udi);

    QString exec = m_service.exec();
    MacroExpander mx(device);
    mx.expandMacrosShellQuote(exec);

    KIO::CommandLauncherJob *job = new KIO::CommandLauncherJob(exec);
    job->setIcon(m_service.icon());
    job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));

    // To make xdg-activation and startup feedback work we need to pass the desktop file name of what we are launching
    if (m_service.service()->storageId().endsWith(QLatin1String("test-predicate-openinwindow.desktop"))) {
        // We know that we are going to launch the default file manager, so query the desktop file name of that
        const KService::Ptr defaultFileManager = KApplicationTrader::preferredService(QStringLiteral("inode/directory"));
        job->setDesktopName(defaultFileManager->desktopEntryName());
    } else {
        // Read the app that will be launched from the desktop file
        KDesktopFile desktopFile(m_service.service()->storageId());
        job->setDesktopName(desktopFile.desktopGroup().readEntry("X-KDE-AliasFor"));
    }

    job->start();

    deleteLater();
}

#include "deviceserviceaction.moc"
