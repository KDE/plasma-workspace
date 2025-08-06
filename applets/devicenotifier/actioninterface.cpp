/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 * SPDX-FileCopyrightText: 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
 * SPDX-FileCopyrightText: 2005-2007 Kevin Ottens <ervin@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "actioninterface.h"

#include <QStandardPaths>
#include <devicenotifier_debug.h>

#include <KApplicationTrader>
#include <KConfigGroup>
#include <KDesktopFile>
#include <KIO/CommandLauncherJob>
#include <KNotificationJobUiDelegate>
#include <KService>
#include <kmacroexpander.h>

#include <Solid/Block>
#include <Solid/Device>

class MacroExpander : public KMacroExpanderBase
{
public:
    explicit MacroExpander(const std::shared_ptr<StorageInfo> &storageInfo)
        : KMacroExpanderBase(QLatin1Char('%'))
        , m_storageInfo(storageInfo)
    {
    }

protected:
    int expandEscapedMacro(const QString &str, int pos, QStringList &ret) override;

private:
    std::shared_ptr<StorageInfo> m_storageInfo;
};

int MacroExpander::expandEscapedMacro(const QString &str, int pos, QStringList &ret)
{
    const Solid::Device &device = m_storageInfo->device();

    ushort option = str[pos + 1].unicode();

    switch (option) {
    case 'f': // Filepath
    case 'F': // case insensitive
        if (device.is<Solid::StorageAccess>()) {
            ret << device.as<Solid::StorageAccess>()->filePath();
        } else {
            qCWarning(APPLETS::DEVICENOTIFIER) << "DeviceServiceAction::execute: " << device.udi() << " is not a StorageAccess device";
        }
        break;
    case 'd': // Device node
    case 'D': // case insensitive
        if (device.is<Solid::Block>()) {
            ret << device.as<Solid::Block>()->device();
        } else {
            qCWarning(APPLETS::DEVICENOTIFIER) << "DeviceServiceAction::execute: " << device.udi() << " is not a Block device";
        }
        break;
    case 'i': // UDI
    case 'I': // case insensitive
        ret << device.udi();
        break;
    case 'j': // Last section of UDI
    case 'J': // case insensitive
        ret << device.udi().section(QLatin1Char('/'), -1);
        break;
    case '%':
        ret = QStringList(QLatin1String("%"));
        break;
    default:
        return -2; // subst with same and skip
    }
    return 2;
}

ActionInterface::ActionInterface(const std::shared_ptr<StorageInfo> &storageInfo, const std::shared_ptr<StateInfo> &stateInfo, QObject *parent)
    : QObject(parent)
    , m_storageInfo(storageInfo)
    , m_stateInfo(stateInfo)
{
}

ActionInterface::~ActionInterface() = default;

QString ActionInterface::predicate() const
{
    return {};
}

void ActionInterface::triggered()
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Default action triggered: " << predicate();
    const QString filePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, u"solid/actions/" + predicate());

    auto services = KService(filePath).actions();
    if (services.size() < 1) {
        qWarning() << "Failed to resolve hotplugjob action" << predicate() << filePath;
        return;
    }
    // Cannot be > 1, we only have one filePath, and < 1 was handled as error.
    Q_ASSERT(services.size() == 1);

    m_service = std::make_unique<KServiceAction>(services.takeFirst());

    auto device = m_storageInfo->device();
    // If this device is an accessible (aka mountable) storage device and it's available
    const bool storageIsAccessible = device.is<Solid::StorageAccess>() && !device.as<Solid::StorageAccess>()->isAccessible();
    // Skip mounting for actions that explicitly request we don't do that (e.g. editing partitions)
    const bool mountingRequired = !m_service->service()->property<bool>(QStringLiteral("X-KDE-SkipMount"));
    if (storageIsAccessible && mountingRequired) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "DelayedExecutor " << m_storageInfo->device().udi() << " : Device is not mounted. Mounting";

        connect(m_stateInfo.get(), &StateInfo::stateChanged, this, &ActionInterface::storageSetupDone);

        device.as<Solid::StorageAccess>()->setup();
    } else {
        qCDebug(APPLETS::DEVICENOTIFIER) << "DelayedExecutor " << m_storageInfo->device().udi() << " : Triggering action";
        delayedExecute();
    }
}

bool ActionInterface::isValid() const
{
    qCWarning(APPLETS::DEVICENOTIFIER) << "Action: " << predicate() << " not valid";
    return false;
}

void ActionInterface::storageSetupDone(const QString &udi)
{
    Q_UNUSED(udi);

    if (m_stateInfo->getState() == StateInfo::MountDone) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "DelayedExecutor " << m_storageInfo->device().udi() << " : Mounting finished";
        if (m_stateInfo->isMounted()) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "DelayedExecutor " << m_storageInfo->device().udi() << " : Mounting successful. Triggering action";
            delayedExecute();
        }
        m_service.reset();
    }
}

void ActionInterface::delayedExecute()
{
    QString exec = m_service->exec();
    MacroExpander mx(m_storageInfo);
    mx.expandMacrosShellQuote(exec);

    auto *job = new KIO::CommandLauncherJob(exec);
    job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));

    // To make xdg-activation and startup feedback work we need to pass the desktop file name of what we are launching
    if (m_service->service()->storageId().endsWith(QLatin1String("openWithFileManager.desktop"))) {
        // We know that we are going to launch the default file manager, so query the desktop file name of that
        const KService::Ptr defaultFileManager = KApplicationTrader::preferredService(QStringLiteral("inode/directory"));
        if (defaultFileManager) [[likely]] {
            job->setDesktopName(defaultFileManager->desktopEntryName());
        }
    } else {
        // Read the app that will be launched from the desktop file
        KDesktopFile desktopFile(m_service->service()->storageId());
        job->setDesktopName(desktopFile.desktopGroup().readEntry("X-KDE-AliasFor"));
    }

    job->start();
}

#include "moc_actioninterface.cpp"
