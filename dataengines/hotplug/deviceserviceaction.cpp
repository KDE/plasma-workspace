/* This file is part of the KDE Project
   Copyright (c) 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
   Copyright (c) 2005-2007 Kevin Ottens <ervin@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "deviceserviceaction.h"

#include <QDebug>

#include <KIO/CommandLauncherJob>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <kmacroexpander.h>
#include <solid/block.h>
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

private slots:
    void _k_storageSetupDone(Solid::ErrorType error, QVariant errorData, const QString &udi);

private:
    void delayedExecute(const QString &udi);

    KServiceAction m_service;
};

DeviceServiceAction::DeviceServiceAction()
    : DeviceAction()
{
    DeviceAction::setIconName(QStringLiteral("dialog-cancel"));
    DeviceAction::setLabel(i18nc("A default name for an action without proper label", "Unknown"));
}

QString DeviceServiceAction::id() const
{
    if (m_service.name().isEmpty() && m_service.exec().isEmpty()) {
        return QString();
    } else {
        return "#Service:" + m_service.name() + m_service.exec();
    }
}

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
    DeviceAction::setIconName(service.icon());
    DeviceAction::setLabel(service.text());

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
    job->start();

    deleteLater();
}

#include "deviceserviceaction.moc"
